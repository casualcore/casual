//!
//! queue.cpp
//!
//! Created on: Jun 6, 2014
//!     Author: Lazan
//!

#include "queue/group/group.h"
#include "queue/group/handle.h"

#include "queue/environment.h"

#include "common/message/dispatch.h"


namespace casual
{
   namespace queue
   {

      namespace group
      {

         Server::Server( Settings settings) : m_state( std::move( settings.queuebase))
         {
            //
            // Talk to queue-broker to get configuration
            //

            group::queue::blocking::Writer queueBroker{ environment::broker::queue::id(), m_state};

            {
               common::message::queue::connect::Request request;
               request.server = common::message::server::Id::current();
               queueBroker( request);
            }

            {
               std::vector< std::string> existing;
               for( auto&& queue : m_state.queuebase.queues())
               {
                  existing.push_back( queue.name);
               }


               group::queue::blocking::Reader read( common::ipc::receive::queue(), m_state);
               common::message::queue::connect::Reply reply;
               read( reply);

               std::vector< std::string> added;

               for( auto&& queue : reply.queues)
               {
                  m_state.queuebase.create( Queue{ queue.name, queue.retries});
                  added.push_back( queue.name);
               }


               //
               // Try to remove queues
               // TODO:
               //
               auto removed = common::range::difference( existing, added);


               //
               // Send all our queues to queue-broker
               //
               common::message::queue::Information information;
               information.queues = m_state.queuebase.queues();

               queueBroker( information);

            }


         }


         void Server::start()
         {
            common::message::dispatch::Handler handler;

            handler.add( handle::enqueue::Request{ m_state});
            handler.add( handle::dequeue::Request{ m_state});

            group::queue::blocking::Reader blockedRead( common::ipc::receive::queue(), m_state);

            while( true)
            {
               handler.dispatch( blockedRead.next());
            }

         }
      } // server

   } // queue

} // casual
