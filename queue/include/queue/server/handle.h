//!
//! handle.h
//!
//! Created on: Jun 14, 2014
//!     Author: Lazan
//!

#ifndef QUEUE_SERVER_HANDLE_H_
#define QUEUE_SERVER_HANDLE_H_

#include "queue/server/server.h"

#include "common/message/queue.h"

namespace casual
{

   namespace queue
   {
      namespace server
      {
         namespace handle
         {
            struct Base
            {
               Base( State& state) : m_state( state) {}

            protected:
               State& m_state;

            };

            namespace enqueue
            {
               struct Request : Base
               {
                  using message_type = common::message::queue::enqueue::Request;

                  using Base::Base;

                  void dispatch( message_type& message);
               };

            } // enqueue

            namespace dequeue
            {
               struct Request : Base
               {
                  using message_type = common::message::queue::dequeue::Request;

                  using Base::Base;

                  void dispatch( message_type& message);
               };

               struct Reply : Base
               {
                  using message_type = common::message::queue::dequeue::Reply;

                  using Base::Base;

                  void dispatch( message_type& message);
               };

            } // dequeue


         } // handle

      } // server
   } // queue


} // casual

#endif // HANDLE_H_
