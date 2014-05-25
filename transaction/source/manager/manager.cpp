//!
//! monitor.cpp
//!
//! Created on: Jul 15, 2013
//!     Author: Lazan
//!

#include "transaction/manager/manager.h"
#include "transaction/manager/handle.h"
#include "transaction/manager/action.h"


#include "common/server_context.h"
#include "common/message.h"
#include "common/trace.h"
#include "common/queue.h"
#include "common/environment.h"
#include "common/message_dispatch.h"
#include "common/log.h"

#include "config/domain.h"

#include "sf/archive_maker.h"


#include <tx.h>

using namespace casual::common;


extern "C"
{
   extern void casual_listTransactions( TPSVCINFO *serviceInfo);
}


namespace casual
{
   namespace transaction
   {


      State::State( const std::string& database) : log( database) {}


      Manager::Manager( const Settings& settings) :
          m_receiveQueue( ipc::receive::queue()),
          m_state( settings.database)
      {

      }

      Manager::~Manager()
      {
         try
         {
            common::trace::Exit temp( "terminate child processes");

            //
            // We need to terminate all children
            //
            /*
            for( auto& resource : m_state.resources)
            {
               for( auto& instances : resource.servers)
               {
                  log::information << "terminate: " << instances.id.pid;
                  process::terminate( instances.id.pid);
               }
            }
             */

            for( auto death : process::lifetime::ended())
            {
               log::information << "shutdown: " << death.string();
            }

         }
         catch( ...)
         {
            common::error::handler();
         }

      }

      void Manager::start()
      {
         auto start = common::platform::clock_type::now();

         common::log::internal::transaction << "transaction manager start\n";


         queue::blocking::Reader queueReader{ m_receiveQueue, m_state};


         //
         // Connect and get configuration from broker
         //
         {
            common::log::internal::transaction << "configure\n";

            queue::blocking::Writer brokerQueue{ ipc::broker::id(), m_state};
            action::configure( m_state, brokerQueue, queueReader);
         }

         //
         // Start resource-proxies
         //
         {
            common::log::internal::transaction << "start rm-proxy-servers\n";

            common::range::for_each(
               common::range::make( m_state.resources),
               action::boot::Proxie( m_state));

         }


         common::log::internal::transaction << "prepare message dispatch handlers\n";

         //
         // prepare message dispatch handlers...
         //

         message::dispatch::Handler handler;

         handler.add( handle::Begin{ m_state});
         handler.add( handle::Commit{ m_state});
         handler.add( handle::Rollback{ m_state});
         handler.add( handle::resource::Involved{ m_state});
         handler.add( handle::resource::reply::Connect( m_state, ipc::broker::id()));
         handler.add( handle::resource::reply::Prepare{ m_state});
         handler.add( handle::resource::reply::Commit{ m_state});
         handler.add( handle::resource::reply::Rollback{ m_state});
         handler.add( handle::domain::Prepare{ m_state});
         handler.add( handle::domain::Commit{ m_state});
         handler.add( handle::domain::Rollback{ m_state});
         handler.add( handle::domain::resource::reply::Prepare{ m_state});
         handler.add( handle::domain::resource::reply::Commit{ m_state});
         handler.add( handle::domain::resource::reply::Rollback{ m_state});

         //
         // Prepare the xatmi-services
         //
         {
            common::server::Arguments arguments{ { common::process::path()}};

            arguments.services.emplace_back( "casual-listTransactions", &casual_listTransactions, 10, common::server::Service::cNone);

            handler.add( handle::admin::Call{ arguments, m_state});

            //
            // We discard the connect reply message
            //
            handler.add( common::message::dispatch::Discard< common::message::server::connect::Reply>{});

         }



         common::log::internal::transaction << "start message pump\n";



         auto instances = common::range::accumulate(
               m_state.resources, 0,
               []( std::size_t count, const state::resource::Proxy& p)
               {
                  return count + p.instances.size();
               }
               );

         auto end = common::platform::clock_type::now();


         common::log::information << "transaction manager up and running - "
               << m_state.resources.size() << " resources - "
               << instances << " instances - boot time: "
               << std::chrono::duration_cast< std::chrono::milliseconds>( end - start).count() << " ms" << std::endl;


         while( true)
         {
            try
            {
               try
               {
                  scoped::Writer batchWrite( m_state.log);

                  //
                  // Blocking
                  //

                  handler.dispatch( queueReader.next());


                  //
                  // Consume until the queue is empty or we've got pending replies equal to transaction_batch
                  //

                  queue::non_blocking::Reader nonBlocking( m_receiveQueue, m_state);

                  while( handler.dispatch( nonBlocking.next()) &&
                        m_state.persistentReplies.size() < common::platform::transaction_batch)
                  {
                     ;
                  }
               }
               catch( ...)
               {
                  throw;
               }


               //
               // Send persistent replies to clients
               //
               {
                  common::log::internal::transaction << "manager persistent replies: " << m_state.persistentReplies.size() << "\n";

                  auto notDone = common::range::partition(
                        m_state.persistentReplies,
                        common::negate( action::persistent::Send{ m_state}));

                  common::range::trim( m_state.persistentReplies, notDone);
               }

               //
               // Send persistent resource requests
               //
               {
                  common::log::internal::transaction << "manager persistent request: " << m_state.persistentRequests.size() << "\n";

                  auto notDone = common::range::partition(
                        m_state.persistentRequests,
                        common::negate( action::persistent::Send{ m_state}));

                  //
                  // Move the ones that did not find an idle resource to pending requests
                  //
                  common::range::move( notDone, m_state.pendingRequests);

                  m_state.persistentRequests.clear();

               }

            }
            catch( ...)
            {
               //error::handler();
               throw;
            }

         }
      }


   } // transaction
} // casual


