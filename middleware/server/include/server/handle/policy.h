//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "server/argument.h"
//#include "common/server/context.h"
#include "server/service/invoke.h"


#include "common/message/event.h"
#include "common/message/service.h"
#include "common/message/conversation.h"

namespace casual
{
   namespace server::handle::policy
   {
      void advertise( std::vector< server::Service> services);

      namespace call
      {
         //! Default policy for basic_call.
         struct Default
         {
            void configure( server::Arguments&& arguments);

            void reply( common::strong::ipc::id id, common::message::service::call::Reply& message);
            void reply( common::strong::ipc::id id, common::message::conversation::callee::Send& message);

            void ack( const common::message::service::call::ACK& message);

            void statistics( common::strong::ipc::id id, common::message::event::service::Call& event);

            void transaction(
                  const common::transaction::ID& trid,
                  const server::Service& service);


            common::message::service::transaction::State transaction( bool commit);

            void forward( service::invoke::Forward&& forward, const common::message::service::call::callee::Request& message);
            void forward( service::invoke::Forward&& forward, const common::message::conversation::connect::callee::Request& message);
         };


         struct Admin
         {
            void configure( server::Arguments&& arguments);
            void reply( common::strong::ipc::id id, common::message::service::call::Reply& message);
            void ack( const common::message::service::call::ACK& message);
            void statistics( common::strong::ipc::id id, common::message::event::service::Call& event);

            common::message::service::transaction::State transaction( bool commit);

            void transaction(
                  const common::transaction::ID& trid,
                  const server::Service& service);


            void forward( service::invoke::Forward&& forward, const common::message::service::call::callee::Request& message);
         };

      } // call

   } // server::handle::policy
} // casual


