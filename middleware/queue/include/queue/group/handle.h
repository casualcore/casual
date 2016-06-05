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
#include "common/message/domain.h"

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


         namespace handle
         {

            void shutdown( State& state);

            namespace dead
            {
               struct Process : Base
               {
                  using Base::Base;

                  void operator() ( const common::message::domain::process::termination::Event& message);
               };
            } // dead

            namespace information
            {

               namespace queues
               {

                  struct Request : Base
                  {

                     using message_type = common::message::queue::information::queues::Request;

                     using Base::Base;

                     void operator () ( message_type& message);
                  };

               } // queues

               namespace messages
               {
                  struct Request : Base
                  {
                     using message_type = common::message::queue::information::messages::Request;

                     using Base::Base;

                     void operator () ( message_type& message);
                  };

               } // messages


            } // information

            namespace enqueue
            {
               struct Request : Base
               {
                  using message_type = common::message::queue::enqueue::Request;

                  using Base::Base;

                  void operator () ( message_type& message);
               };

            } // enqueue

            namespace dequeue
            {

               struct Request : Base
               {
                  using message_type = common::message::queue::dequeue::Request;

                  using Base::Base;

                  void operator () ( message_type& message);
               };

               bool request( State& state, Request::message_type& message);

               namespace forget
               {
                  struct Request : Base
                  {
                     using message_type = common::message::queue::dequeue::forget::Request;

                     using Base::Base;

                     void operator () ( message_type& message);

                  };

               } // forget

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

                     void operator () ( message_type& message);

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

                     void operator () ( message_type& message);

                  };
               }
            }

         } // handle
      } // group
   } // queue


} // casual

#endif // HANDLE_H_
