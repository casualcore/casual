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
#include "common/queue.h"

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
         }

         namespace queue
         {

            struct Policy : public handle::Base
            {
               using handle::Base::Base;

               void apply();
            };


            namespace blocking
            {
               using Reader = common::queue::blocking::basic_reader< Policy>;
               using Writer = common::queue::blocking::basic_writer< Policy>;

            } // blocking

            namespace non_blocking
            {
               using Reader = common::queue::non_blocking::basic_reader< Policy>;
               using Writer = common::queue::non_blocking::basic_writer< Policy>;

            } // non_blocking
         } // queue


         namespace handle
         {

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


            } // dequeue
         } // handle
      } // server
   } // queue


} // casual

#endif // HANDLE_H_
