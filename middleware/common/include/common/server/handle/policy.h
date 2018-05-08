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
   namespace common
   {
      namespace server
      {
         namespace handle
         {
            namespace policy
            {
               namespace call
               {

                  //!
                  //! Default policy for basic_call. Only broker and unittest have to define another
                  //! policy
                  //!
                  struct Default
                  {
                     void configure( server::Arguments& arguments);

                     void reply( strong::ipc::id id, message::service::call::Reply& message);
                     void reply( strong::ipc::id id, message::conversation::caller::Send& message);

                     void ack();

                     void statistics( strong::ipc::id id, message::event::service::Call& event);

                     void transaction(
                           const common::transaction::ID& trid,
                           const server::Service& service,
                           const common::platform::time::unit& timeout,
                           const platform::time::point::type& now);


                     message::service::Transaction transaction( bool commit);

                     void forward( common::service::invoke::Forward&& forward, const message::service::call::callee::Request& message);
                     void forward( common::service::invoke::Forward&& forward, const message::conversation::connect::callee::Request& message);
                  };


                  struct Admin
                  {
                     Admin( communication::error::type handler);

                     void configure( server::Arguments& arguments);
                     void reply( strong::ipc::id id, message::service::call::Reply& message);
                     void ack();
                     void statistics( strong::ipc::id id, message::event::service::Call& event);

                     message::service::Transaction transaction( bool commit);

                     void transaction(
                           const common::transaction::ID& trid,
                           const server::Service& service,
                           const common::platform::time::unit& timeout,
                           const platform::time::point::type& now);


                     void forward( common::service::invoke::Forward&& forward, const message::service::call::callee::Request& message);

                  private:
                     communication::error::type m_error_handler;
                  };

               } // call

            } // policy

         } // handle
      } // server
   } // common
} // casual


