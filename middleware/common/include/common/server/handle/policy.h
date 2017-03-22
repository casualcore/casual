//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVER_HANDLE_POLICY_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVER_HANDLE_POLICY_H_

#include "common/server/argument.h"
#include "common/server/context.h"
#include "common/communication/device.h"

#include "common/message/traffic.h"
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

                     void reply( platform::ipc::id::type id, message::service::call::Reply& message);
                     void reply( platform::ipc::id::type id, message::conversation::caller::Send& message);

                     void ack( const message::service::call::callee::Request& message);
                     void ack( const message::conversation::connect::callee::Request& message);



                     void statistics( platform::ipc::id::type id, message::traffic::Event& event);

                     void transaction( const message::service::call::callee::Request& message, const server::Service& service, const platform::time::point::type& now);
                     void transaction( const message::conversation::connect::callee::Request& message, const server::Service& service, const platform::time::point::type& now);

                     void transaction( message::service::call::Reply& message, int return_state);
                     void transaction( message::conversation::caller::Send& message, int return_state);


                     void forward( const message::service::call::callee::Request& message, const state::Jump& jump);
                     void forward( const message::conversation::connect::callee::Request& message, const state::Jump& jump);
                  };


                  struct Admin
                  {
                     Admin( communication::error::type handler);


                     void configure( server::Arguments& arguments);

                     void reply( platform::ipc::id::type id, message::service::call::Reply& message);

                     void ack( const message::service::call::callee::Request& message);


                     void statistics( platform::ipc::id::type id, message::traffic::Event&);

                     void transaction( const message::service::call::callee::Request&, const server::Service&, const common::platform::time::point::type&);

                     void transaction( message::service::call::Reply& message, int return_state);

                     void forward( const common::message::service::call::callee::Request& message, const state::Jump& jump);

                  private:
                     communication::error::type m_error_handler;
                  };

               } // call

            } // policy

         } // handle
      } // server
   } // common
} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVER_HANDLE_POLICY_H_
