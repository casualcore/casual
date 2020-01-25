//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "queue/group/group.h"

#include "common/message/queue.h"
#include "common/message/signal.h"
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

            namespace ipc
            {
               common::communication::ipc::inbound::Device& device();
            }

            struct Base
            {
               inline Base( State& state) : m_state( state) {}
            protected:
               State& m_state;
            };
         }


         namespace handle
         {
            using dispatch_type = common::communication::ipc::dispatch::Handler;

            void shutdown( State& state);


            //! * persist the queuebase
            //! * send pending replies, if any
            //! * check if pending request has some messages to consume
            void persist( State& state);

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
                     using Base::Base;
                     void operator () ( common::message::queue::information::queues::Request& message);
                  };

               } // queues

               namespace messages
               {
                  struct Request : Base
                  {
                     using Base::Base;
                     void operator () ( common::message::queue::information::messages::Request& message);
                  };

               } // messages

            } // information

            namespace enqueue
            {
               struct Request : Base
               {
                  using Base::Base;
                  void operator () ( common::message::queue::enqueue::Request& message);
               };

            } // enqueue

            namespace dequeue
            {
               struct Request : Base
               {
                  using Base::Base;

                  //! @returns false if message is added to pending, true otherwise
                  bool operator () ( common::message::queue::dequeue::Request& message);

               private:
                  bool handle( common::message::queue::dequeue::Request& message);
               };

               

               namespace forget
               {
                  struct Request : Base
                  {
                     using Base::Base;
                     void operator () ( common::message::queue::dequeue::forget::Request& message);
                  };

               } // forget

            } // dequeue

            namespace peek
            {
               namespace information
               {
                  struct Request : Base
                  {
                     using Base::Base;
                     void operator () ( common::message::queue::peek::information::Request& message);
                  };
               } // information

               namespace messages
               {
                  struct Request : Base
                  {
                     using Base::Base;
                     void operator () ( common::message::queue::peek::messages::Request& message);
                  };
               } // messages

            } // peek


            namespace transaction
            {
               namespace commit
               {
                  //! Invoked from the TM
                  struct Request : Base
                  {
                     using Base::Base;
                     void operator () ( common::message::transaction::resource::commit::Request& message);
                  };
               }

               namespace prepare
               {
                  //! Invoked from the TM
                  //!
                  //! This will always reply ok.
                  struct Request : Base
                  {
                     using Base::Base;
                     void operator () ( common::message::transaction::resource::prepare::Request& message);
                  };
               }

               namespace rollback
               {
                  //! Invoked from the TM
                  struct Request : Base
                  {
                     using Base::Base;
                     void operator () ( common::message::transaction::resource::rollback::Request& message);
                  };
               }
            } // transaction

            namespace restore
            {
               struct Request : Base
               {
                  using Base::Base;
                  void operator () ( common::message::queue::restore::Request& message);
               };

            } // restore

            namespace signal
            {
               struct Timeout : Base
               {
                  using Base::Base;
                  void operator () ( const common::message::signal::Timeout&);
               };
            } // signal

         } // handle

         handle::dispatch_type handler( State& state);

      } // group
   } // queue


} // casual


