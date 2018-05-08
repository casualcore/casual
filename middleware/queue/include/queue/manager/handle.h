//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "queue/manager/manager.h"

#include "common/message/queue.h"
#include "common/message/transaction.h"
#include "common/message/gateway.h"
#include "common/message/domain.h"
#include "common/message/event.h"
#include "common/message/dispatch.h"
#include "common/communication/ipc.h"

namespace casual
{
   namespace queue
   {
      namespace manager
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

                  using message_type = common::message::event::process::Exit;

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
                  using Base::Base;
                  void operator () ( common::message::queue::connect::Request& message);
               };

               struct Information : Base
               {
                  using Base::Base;
                  void operator () ( common::message::queue::Information& message);
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


