//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef QUEUE_SERVER_HANDLE_H_
#define QUEUE_SERVER_HANDLE_H_

#include "queue/group/group.h"

#include "common/message/queue.h"
#include "common/message/dispatch.h"
#include "common/message/transaction.h"
#include "common/message/event.h"

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
            using dispatch_type = common::communication::ipc::dispatch::Handler;

            void shutdown( State& state);

            namespace dead
            {
               struct Process : Base
               {
                  using Base::Base;

                  void operator() ( const common::message::event::process::Exit& message);
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

            namespace peek
            {
               namespace information
               {
                  struct Request : Base
                  {
                     using message_type = common::message::queue::peek::information::Request;

                     using Base::Base;

                     void operator () ( message_type& message);
                  };
               } // information

               namespace messages
               {
                  struct Request : Base
                  {
                     using message_type = common::message::queue::peek::messages::Request;

                     using Base::Base;

                     void operator () ( message_type& message);
                  };
               } // messages

            } // peek


            namespace transaction
            {
               namespace commit
               {
                  //!
                  //! Invoked from the TM
                  //!
                  struct Request : Base
                  {
                     using message_type = common::message::transaction::resource::commit::Request;

                     using Base::Base;

                     void operator () ( message_type& message);

                  };
               }

               namespace prepare
               {
                  //!
                  //! Invoked from the TM
                  //!
                  //! This will always reply ok.
                  //!
                  struct Request : Base
                  {
                     using message_type = common::message::transaction::resource::prepare::Request;

                     using Base::Base;

                     void operator () ( message_type& message);

                  };
               }

               namespace rollback
               {
                  //!
                  //! Invoked from the TM
                  //!
                  struct Request : Base
                  {
                     using message_type = common::message::transaction::resource::rollback::Request;

                     using Base::Base;

                     void operator () ( message_type& message);

                  };
               }
            } // transaction

            namespace restore
            {
               struct Request : Base
               {
                  using message_type = common::message::queue::restore::Request;

                  using Base::Base;

                  void operator () ( message_type& message);
               };

            } // restore

         } // handle

         handle::dispatch_type handler( State& state);

      } // group
   } // queue


} // casual

#endif // HANDLE_H_
