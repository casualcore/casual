//!
//! monitor.cpp
//!
//! Created on: Jul 15, 2013
//!     Author: Lazan
//!

#include "transaction/manager/manager.h"
#include "transaction/manager/handle.h"
#include "transaction/manager/action.h"
#include "transaction/manager/admin/server.h"


#include "common/server/handle.h"
#include "common/server/lifetime.h"
#include "common/trace.h"
#include "common/queue.h"
#include "common/environment.h"
#include "common/message/dispatch.h"
#include "common/message/handle.h"
#include "common/log.h"


#include "config/domain.h"


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
          m_queueFilePath( common::process::singleton( common::environment::domain::singleton::path() + "/.casual-transaction-manager-queue")),
          m_receiveQueue( ipc::receive::queue()),
          m_state( settings.database)
      {

      }

      Manager::~Manager()
      {
         try
         {
            std::vector< common::process::Handle> instances;

            for( auto& resource : m_state.resources)
            {
               common::range::transform( resource.instances, instances, std::mem_fn( &state::resource::Proxy::Instance::process));
            }

            common::server::lifetime::shutdown( m_state, instances, {}, std::chrono::seconds( 1));

            auto pids  = m_state.processes();

            if( pids.size() != 0)
            {
               common::log::error << "failed to shutdown resource proxies: " << range::make( pids) << " - action: abort" << std::endl;
            }
            else
            {
               common::log::information << "casual-transaction-manager off-line\n";
            }

         }
         catch( ...)
         {
            common::error::handler();
         }

      }

      void Manager::start()
      {
         try
         {
            auto start = common::platform::clock_type::now();

            common::log::internal::transaction << "transaction manager start\n";



            //
            // Connect and get configuration from broker
            //
            {
               trace::internal::Scope trace( "configure", common::log::internal::transaction);
               action::configure( m_state);
            }

            //
            // Start resource-proxies
            //
            {
               trace::internal::Scope trace( "start rm-proxy-servers", common::log::internal::transaction);

               common::range::for_each(
                  common::range::make( m_state.resources),
                  action::boot::Proxie( m_state));

            }


            common::log::internal::transaction << "prepare message dispatch handlers\n";

            //
            // prepare message dispatch handlers...
            //

            message::dispatch::Handler handler{
               common::message::handle::Shutdown{},
               handle::Begin{ m_state},
               handle::Commit{ m_state},
               handle::Rollback{ m_state},
               handle::resource::Involved{ m_state},
               handle::resource::reply::Connect{ m_state},
               handle::resource::reply::Prepare{ m_state},
               handle::resource::reply::Commit{ m_state},
               handle::resource::reply::Rollback{ m_state},
               handle::domain::Prepare{ m_state},
               handle::domain::Commit{ m_state},
               handle::domain::Rollback{ m_state},
               handle::domain::resource::reply::Prepare{ m_state},
               handle::domain::resource::reply::Commit{ m_state},
               handle::domain::resource::reply::Rollback{ m_state},
               common::server::handle::basic_admin_call< State>{ admin::Server::services( *this), m_state},
               common::message::handle::ping( m_state),

               //
               // We discard the connect reply message
               //
               common::message::handle::Discard< common::message::server::connect::Reply>{},
            };



            common::log::internal::transaction << "start message pump\n";



            auto instances = common::range::accumulate(
                  m_state.resources, 0,
                  []( std::size_t count, const state::resource::Proxy& p)
                  {
                     return count + p.instances.size();
                  }
                  );

            auto end = common::platform::clock_type::now();


            common::log::information << "transaction manager is on-line - "
                  << m_state.resources.size() << " resources - "
                  << instances << " instances - boot time: "
                  << std::chrono::duration_cast< std::chrono::milliseconds>( end - start).count() << " ms" << std::endl;


            queue::blocking::Reader queueReader{ m_receiveQueue, m_state};


            while( true)
            {
               {
                  scoped::Writer batchWrite( m_state.log);

                  //
                  // Blocking
                  //

                  handler( queueReader.next());


                  //
                  // Consume until the queue is empty or we've got pending replies equal to transaction_batch
                  //

                  queue::non_blocking::Reader nonBlocking( m_receiveQueue, m_state);

                  while( handler( nonBlocking.next()) &&
                        m_state.persistentReplies.size() < common::platform::batch::transaction)
                  {
                     ;
                  }
               }

               //
               // Send persistent replies to clients
               //
               {
                  common::log::internal::transaction << "manager persistent replies: " << m_state.persistentReplies.size() << "\n";

                  auto notDone = common::range::partition(
                        m_state.persistentReplies,
                        common::negate( action::persistent::Send{ m_state}));

                  common::range::trim( m_state.persistentReplies, std::get< 0>( notDone));
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
                  common::range::move( std::get< 0>( notDone), m_state.pendingRequests);

                  m_state.persistentRequests.clear();

               }
            }

         }
         catch( const common::exception::signal::Terminate&)
         {
            // we do nothing, and let the dtor take care of business
         }
         catch( ...)
         {
            common::error::handler();
         }
      }


   } // transaction
} // casual


