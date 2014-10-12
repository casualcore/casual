//!
//! handle.h
//!
//! Created on: Jun 14, 2014
//!     Author: Lazan
//!

#ifndef QUEUE_SERVER_HANDLE_H_
#define QUEUE_SERVER_HANDLE_H_

#include "queue/group/group.h"

#include "common/message/queue.h"
#include "common/message/transaction.h"
#include "common/queue.h"

namespace casual
{

   namespace queue
   {
      namespace group
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

               using Send = common::queue::non_blocking::basic_send< Policy>;

            } // non_blocking
         } // queue


         namespace handle
         {

            namespace information
            {

               namespace queues
               {

                  struct Request : Base
                  {

                     using message_type = common::message::queue::information::queues::Request;

                     using Base::Base;

                     void dispatch( message_type& message);
                  };

               } // queues

               namespace messages
               {
                  struct Request : Base
                  {
                     using message_type = common::message::queue::information::queue::Request;

                     using Base::Base;

                     void dispatch( message_type& message);
                  };

               } // messages


            } // information

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


            namespace transaction
            {
               namespace commit
               {
                  //!
                  //! Invoked from the casual-queue-broker
                  //!
                  struct Request : Base
                  {
                     using message_type = common::message::transaction::resource::commit::Request;

                     using Base::Base;

                     void dispatch( message_type& message);

                  };
               }

               namespace rollback
               {
                  //!
                  //! Invoked from the casual-queue-broker
                  //!
                  struct Request : Base
                  {
                     using message_type = common::message::transaction::resource::rollback::Request;

                     using Base::Base;

                     void dispatch( message_type& message);

                  };
               }
            }

         } // handle
      } // group
   } // queue


} // casual

#endif // HANDLE_H_
