//!
//! handle.cpp
//!
//! Created on: Jun 20, 2014
//!     Author: Lazan
//!

#include "queue/broker/handle.h"


namespace casual
{
   namespace queue
   {
      namespace broker
      {
         namespace handle
         {

            namespace lookup
            {

               void Request::dispatch( message_type& message)
               {
                  queue::blocking::Writer write{ message.server.queue_id, m_state};

                  auto found = m_state.queues.find( message.name);

                  if( found != std::end( m_state.queues))
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

         } // handle
      } // broker
   } // queue
} // casual
