//!
//! handle.cpp
//!
//! Created on: Jun 14, 2014
//!     Author: Lazan
//!

#include "queue/server/handle.h"

namespace casual
{

   namespace queue
   {
      namespace server
      {
         namespace handle
         {

            namespace enqueue
            {
               void Request::dispatch( message_type& message)
               {
                  m_state.queuebase.enqueue( message);

               }

            } // enqueue

            namespace dequeue
            {
               void Request::dispatch( message_type& message)
               {
               }

               void Reply::dispatch( message_type& message)
               {

               }

            } // dequeue


         } // handle

      } // server
   } // queue


} // casual

