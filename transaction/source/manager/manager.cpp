//!
//! monitor.cpp
//!
//! Created on: Jul 15, 2013
//!     Author: Lazan
//!

#include "transaction/manager/manager.h"
#include "transaction/manager/handle.h"
#include "transaction/manager/action.h"


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

namespace casual
{
   namespace transaction
   {


      State::State( const std::string& database) : log( database) {}


      Manager::Manager( const Settings& settings) :
          m_receiveQueue( ipc::getReceiveQueue()),
          m_state( settings.database)
      {
         common::trace::Exit trace( "transaction manager startup");


         common::log::debug << "transaction manager queue: " << m_receiveQueue.id();

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
         common::Trace trace( "transaction::Manager::start");


         queue::blocking::Reader queueReader{ m_receiveQueue, m_state};
         queue::blocking::Writer brokerQueue{ ipc::broker::id(), m_state};

         //
         // Connect and get configuration from broker
         //
         {

            action::configure( m_state, brokerQueue, queueReader);
         }

         //
         // Start resource-proxies
         //
         {
            common::trace::Exit trace( "transaction manager start rm-proxy-servers");

            std::for_each(
               std::begin( m_state.resources),
               std::end( m_state.resources),
               action::boot::Proxie( m_state));

         }


         //
         // prepare message dispatch handlers...
         //

         message::dispatch::Handler handler;

         handler.add( handle::Begin{ m_state});
         handler.add( handle::Commit{ m_state});
         handler.add( handle::Rollback{ m_state});
         handler.add( handle::resource::connect( m_state, brokerQueue));
         handler.add( handle::resource::Prepare{ m_state});
         handler.add( handle::resource::Commit{ m_state});
         handler.add( handle::resource::Rollback{ m_state});


         while( true)
         {
            {
               scoped::Writer batchWrite( m_state.log);

               //
               // Blocking
               //

               handler.dispatch( queueReader.next());


               //
               // Consume until the queue is empty or we've got pending replies equal to transaction_batch
               //
               {

                  queue::non_blocking::Reader nonBlocking( m_receiveQueue, m_state);

                  while( handler.dispatch( nonBlocking.next()) &&
                        m_state.pendingReplies.size() < common::platform::transaction_batch)
                  {
                     ;
                  }

               }

               /*
               std::for_each(
                  std::begin( m_state.pendingReplies),
                  std::end( m_state.pendingReplies),
                  action::Send< QueueBlockingWriter>{ m_state});
                  */

               m_state.pendingReplies.clear();


            }
         }
      }


   } // transaction
} // casual


