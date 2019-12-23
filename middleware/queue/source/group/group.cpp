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
#include "common/event/send.h"

#include "common/communication/instance.h"

namespace casual
{
   using namespace common;
   namespace queue
   {
      namespace group
      {

         Server::Server( Settings settings) 
            : m_state( std::move( settings.queuebase), std::move( settings.name))
         {
            Trace trace{ "queue::group::Server::Server"};

            // Talk to queue-manager to get configuration
            auto reply = communication::ipc::call(  
               common::communication::instance::outbound::queue::manager::device(),
               []()
               {
                  common::message::queue::connect::Request request;
                  request.process = process::handle();
                  return request;
               }());

            auto existing = m_state.queuebase.queues();
            log::line( verbose::log, "existing: ", existing);

            auto correlate_id = [existing = std::move( existing)]( auto& queue)
            {
               if( auto found = algorithm::find( existing, queue.name))
               {
                  queue.id = found->id;
                  queue.error = found->error;
               }
            };

            algorithm::for_each( reply.queues, correlate_id);
            log::line( verbose::log, "reply.queues: ", reply.queues);
            
            // if something goes wrong we send fatal event
            event::guard::fatal( [&]()
            { 
               m_state.queuebase.update( std::move( reply.queues), {});
            });


            // TODO: What to do with existing queues that has messages?
            //  - for now we just leave them as is

            // Send all our queues to queue-manager
            {
               common::message::queue::Information information;
               information.name = m_state.name();
               information.process = process::handle();
               information.queues = m_state.queuebase.queues();

               communication::ipc::blocking::send( communication::instance::outbound::queue::manager::device(), information);
            }
         }

         Server::~Server()
         {
            Trace trace{ "queue::group::Server~Server"};

            common::exception::guard( [&](){
                handle::shutdown( m_state);
            });
         }


         void Server::start()
         {
            Trace trace{ "queue::group::Server::start"};

            auto handler = group::handler( m_state);


            // make sure we persist when inbound
            // is empty, 
            auto empty_inbound = [&]()
            {
               handle::persist( m_state);
            };

            common::message::dispatch::empty::pump( handler, handle::ipc::device(), empty_inbound);
         }

      } // group
   } // queue
} // casual
