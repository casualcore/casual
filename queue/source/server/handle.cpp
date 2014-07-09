//!
//! handle.cpp
//!
//! Created on: Jun 14, 2014
//!     Author: Lazan
//!

#include "queue/server/handle.h"

#include "queue/environment.h"

namespace casual
{

   namespace queue
   {
      namespace server
      {
         namespace queue
         {
            void Policy::apply()
            {

            }

         }

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
                  auto reply = m_state.queuebase.dequeue( message);
                  queue::blocking::Writer write{ environment::broker::queue::id(), m_state};
                  write( reply);
               }


            } // dequeue


         } // handle

      } // server
   } // queue


} // casual

