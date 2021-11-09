//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/server/handle/conversation.h"
#include "common/server/handle/service.h"
#include "common/server/handle/policy.h"

#include "common/service/conversation/context.h"

#include "common/communication/ipc.h"

#include "common/execute.h"

namespace casual
{
   namespace common
   {
      namespace server
      {
         namespace handle
         {

            void Conversation::operator () ( message::conversation::connect::callee::Request& message)
            {
               Trace trace{ "server::handle::Conversation::operator()"};
               log::line( verbose::log, "message: ", message);

               policy::call::Default policy;

               service::call( policy, common::service::conversation::context(), message, true);
   
            }


         } // handle
      } // server
   } // common
} // casual
