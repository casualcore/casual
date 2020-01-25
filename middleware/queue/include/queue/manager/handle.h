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
            common::communication::ipc::inbound::Device& device();
         } // ipc




         namespace handle
         {

            using dispatch_type = common::communication::ipc::dispatch::Handler;

            namespace process
            {
               struct Exit : Base
               {
                  using Base::Base;
                  void operator () ( const common::message::event::process::Exit& message);    
               };

               void exit( const common::process::lifetime::Exit& exit);

            } // process

            namespace shutdown
            {
               struct Request : Base
               {
                  using Base::Base;
                  void operator () ( common::message::shutdown::Request& message);
               };

            } // shutdown


            namespace lookup
            {
               struct Request : Base
               {
                  using Base::Base;
                  void operator () ( common::message::queue::lookup::Request& message);
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

            namespace concurrent
            {
               //!
               //! Handles remote advertise
               //!  - add  0..* queues
               //!  - remove 0..* queues
               //!  - replace == remove all queues for instance and then add 0..* queues
               //!
               struct Advertise : Base
               {
                  using Base::Base;
                  void operator () ( common::message::queue::concurrent::Advertise& message);
               };
            } // concurrent

            namespace domain
            {
               namespace discover
               {
                  struct Request : Base
                  {
                     using Base::Base;
                     void operator () ( common::message::gateway::domain::discover::Request& message);
                  };

                  struct Reply : Base
                  {
                     using Base::Base;
                     void operator () ( common::message::gateway::domain::discover::accumulated::Reply& message);
                  };

               } // discover
            } // domain
         } // handle

         handle::dispatch_type handlers( State& state);

      } // manager
   } // queue
} // casual


