//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "server/handle/conversation.h"
#include "server/handle/call.h"
#include "server/handle/policy.h"

#include "service/conversation/context.h"

#include "common/communication/ipc.h"

#include "common/execute.h"

namespace casual
{

   namespace server::handle
   {

      void Conversation::operator () ( common::message::conversation::connect::callee::Request& message)
      {
         common::Trace trace{ "server::handle::Conversation::operator()"};
         common::log::debug( "message: ", message);

         policy::call::Default policy;

         handle::detail::call( policy, casual::service::conversation::context(), message, true);
      }

   } // server::handle

} // casual
