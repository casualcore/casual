//!
//! casual
//!

#ifndef QUEUE_BROKER_HANDLE_H_
#define QUEUE_BROKER_HANDLE_H_

#include "queue/broker/broker.h"

#include "common/message/queue.h"
#include "common/message/transaction.h"
#include "common/message/gateway.h"
#include "common/message/domain.h"
#include "common/message/dispatch.h"
#include "common/communication/ipc.h"

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

         namespace ipc
         {
            const common::communication::ipc::Helper& device();
         } // ipc




         namespace handle
         {

            using dispatch_type = common::communication::ipc::dispatch::Handler;

            namespace process
            {

               struct Exit : Base
               {
                  using Base::Base;

                  using message_type = common::message::domain::process::termination::Event;

                  void operator () ( message_type& message);

                  void apply( const common::process::lifetime::Exit& exit);
               };

            } // process

            namespace shutdown
            {
               struct Request : Base
               {
                  using message_type = common::message::shutdown::Request;

                  using Base::Base;

                  void operator () ( message_type& message);
               };

            } // shutdown


            namespace lookup
            {
               struct Request : Base
               {
                  using message_type = common::message::queue::lookup::Request;

                  using Base::Base;

                  void operator () ( message_type& message);
               };

            } // lookup

            namespace connect
            {
               struct Request : Base
               {
                  using message_type = common::message::queue::Information;

                  using Base::Base;

                  void operator () ( message_type& message);
               };

            } // connect


            namespace domain
            {
               //!
               //! Handles remote advertise
               //!  - add  0..* queues
               //!  - remove 0..* queues
               //!  - replace == remove all queues for instance and then add 0..* queues
               //!
               struct Advertise : Base
               {
                  using message_type = common::message::gateway::domain::Advertise;
                  using Base::Base;

                  void operator () ( message_type& message);
               };

               namespace discover
               {
                  struct Request : Base
                  {
                     using message_type = common::message::gateway::domain::discover::Request;
                     using Base::Base;

                     void operator () ( message_type& message);
                  };

                  struct Reply : Base
                  {
                     using message_type = common::message::gateway::domain::discover::accumulated::Reply;
                     using Base::Base;

                     void operator () ( message_type& message);
                  };

               } // discover
            } // domain
         } // handle

         handle::dispatch_type handlers( State& state);

      } // broker
   } // queue



} // casual

#endif // HANDLE_H_
