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

         } // handle
      } // broker
   } // queue
} // casual
