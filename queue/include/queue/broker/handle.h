//!
//! handle.h
//!
//! Created on: Jun 20, 2014
//!     Author: Lazan
//!

#ifndef QUEUE_BROKER_HANDLE_H_
#define QUEUE_BROKER_HANDLE_H_

#include "queue/broker/broker.h"

#include "common/message/queue.h"
#include "common/queue.h"

namespace casual
{
   namespace queue
   {
      namespace broker
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


            namespace lookup
            {
               struct Request : Base
               {
                  using message_type = common::message::queue::lookup::Request;

                  using Base::Base;

                  void dispatch( message_type& message);
               };

               struct Reply : Base
               {
                  using message_type = common::message::queue::lookup::Reply;

                  using Base::Base;

                  void dispatch( message_type& message);
               };

            } // lookup

         } // handle
      } // broker
   } // queue



} // casual

#endif // HANDLE_H_
