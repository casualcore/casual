//!
//! handle.cpp
//!
//! Created on: Jun 20, 2014
//!     Author: Lazan
//!

#include "queue/broker/handle.h"


#include "common/log.h"

namespace casual
{
   namespace queue
   {
      namespace broker
      {
         namespace queue
         {
            void Policy::apply()
            {

            }

         }

         namespace handle
         {

            namespace lookup
            {

               void Request::dispatch( message_type& message)
               {
                  queue::blocking::Writer write{ message.server.queue_id, m_state};

                  auto found =  common::range::find( m_state.queues, message.name);

                  if( found)
                  {
                     write( found->second);
                  }
                  else
                  {
                     static const common::message::queue::lookup::Reply reply;
                     write( reply);
                  }
               }

            } // lookup

            namespace connect
            {

               void Request::dispatch( message_type& message)
               {

                  for( auto&& queue : message.queues)
                  {
                     if( ! m_state.queues.emplace( queue.name, common::message::queue::lookup::Reply{ message.server, queue.id}).second)
                     {
                        common::log::error << "multiple instances of queue: " << queue.name << " - action: keeping the first one" << std::endl;
                     }

                  }

               }

            } // connect

         } // handle
      } // broker
   } // queue
} // casual
