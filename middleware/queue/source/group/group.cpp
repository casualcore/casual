//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "queue/group/group.h"
#include "queue/group/handle.h"
#include "queue/common/log.h"

#include "common/message/dispatch.h"
#include "common/message/handle.h"
#include "common/exception/handle.h"

#include "common/communication/instance.h"

namespace casual
{
   namespace queue
   {

      namespace group
      {

         namespace message
         {
            void pump( group::State& state)
            {
               auto handler = group::handler( state);

               common::communication::ipc::Helper ipc;

               while( true)
               {
                  {
                     auto persistent = sql::database::scoped::write( state.queuebase);


                     if( state.persistent.empty())
                     {
                        //
                        // We can only block if our back log is empty...
                        //

                        handler( ipc.blocking_next());
                     }

                     //
                     // Consume until the queue is empty or we've got pending replies equal to transaction_batch
                     //

                     while( handler( ipc.non_blocking_next()) &&
                           state.persistent.size() < common::platform::batch::queue::persitent)
                     {
                        ;
                     }
                  }

                  //
                  // queuebase is persistent - send pending persistent replies
                  //
                  common::algorithm::trim( state.persistent, common::algorithm::remove_if(
                     state.persistent,
                     common::message::pending::sender( common::communication::ipc::policy::non::Blocking{})));

               }

            }
         } // message


         Server::Server( Settings settings) : m_state( std::move( settings.queuebase), std::move( settings.name))
         {
            //
            // Talk to queue-manager to get configuration
            //


            {
               common::message::queue::connect::Request request;
               request.process = common::process::handle();

               common::communication::ipc::blocking::send( common::communication::instance::outbound::queue::manager::device(), request);
            }

            {
               std::vector< std::string> existing;
               for( auto&& queue : m_state.queuebase.queues())
               {
                  existing.push_back( queue.name);
               }


               common::message::queue::connect::Reply reply;
               common::communication::ipc::blocking::receive( common::communication::ipc::inbound::device(), reply);

               std::vector< std::string> added;

               for( auto&& queue : reply.queues)
               {
                  auto exists = common::algorithm::find( existing, queue.name);

                  if( ! exists)
                  {
                     m_state.queuebase.create( Queue{ queue.name, queue.retries});
                     added.push_back( queue.name);
                  }
               }


               //
               // Try to remove queues
               // TODO:
               //
               //auto removed = common::range::difference( existing, added);




               //
               // Send all our queues to queue-manager
               //
               {
                  common::message::queue::Information information;
                  information.name = m_state.name();
                  information.process = common::process::handle();
                  information.queues = m_state.queuebase.queues();

                  common::communication::ipc::blocking::send( common::communication::instance::outbound::queue::manager::device(), information);
               }
            }
         }

         Server::~Server()
         {
            Trace trace{ "queue::server::Server dtor"};

            try
            {
               handle::shutdown( m_state);
            }
            catch( ...)
            {
               common::exception::handle();
            }
         }


         int Server::start() noexcept
         {
            try
            {
               message::pump( m_state);
            }
            catch( ...)
            {
               return common::exception::handle();
            }
            return 0;
         }
      } // server

   } // queue

} // casual
