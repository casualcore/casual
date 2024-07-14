//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/server/argument.h"
#include "common/server/context.h"
#include "common/communication/device.h"

#include "common/message/event.h"
#include "common/message/service.h"
#include "common/message/conversation.h"

namespace casual
{
   namespace common::server::handle::policy
   {
      void advertise( std::vector< server::Service> services);

      namespace call
      {
         //! Default policy for basic_call.
         struct Default
         {
            void configure( server::Arguments&& arguments);

            void reply( strong::ipc::id id, message::service::call::Reply& message);
            void reply( strong::ipc::id id, message::conversation::callee::Send& message);

            void ack( const message::service::call::ACK& message);

            void statistics( strong::ipc::id id, message::event::service::Call& event);

            void transaction(
                  const common::transaction::ID& trid,
                  const server::Service& service,
                  const platform::time::unit& timeout,
                  const platform::time::point::type& now);


            message::service::transaction::State transaction( bool commit);

            void forward( common::service::invoke::Forward&& forward, const message::service::call::callee::Request& message);
            void forward( common::service::invoke::Forward&& forward, const message::conversation::connect::callee::Request& message);
         };


         struct Admin
         {
            void configure( server::Arguments&& arguments);
            void reply( strong::ipc::id id, message::service::call::Reply& message);
            void ack( const message::service::call::ACK& message);
            void statistics( strong::ipc::id id, message::event::service::Call& event);

            message::service::transaction::State transaction( bool commit);

            void transaction(
                  const common::transaction::ID& trid,
                  const server::Service& service,
                  const platform::time::unit& timeout,
                  const platform::time::point::type& now);


            void forward( common::service::invoke::Forward&& forward, const message::service::call::callee::Request& message);
         };

      } // call

   } // common::server::handle::policy
} // casual


