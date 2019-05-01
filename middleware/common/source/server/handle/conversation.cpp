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

            void Conversation::operator () ( message_type& message)
            {
               Trace trace{ "server::handle::Conversation::operator()"};

               log::line( verbose::log, "message: ", message);

               try
               {
                  auto reply = message::reverse::type( message);
                  // We set the worst we got until proven otherwise.
                  reply.code.result = code::xatmi::protocol;

                  auto send_reply = execute::scope( [&](){
                     reply.process = process::handle();
                     reply.recording = message.recording;
                     reply.route = message.recording;

                     auto node = reply.route.next();
                     communication::ipc::blocking::send( node.address, reply);
                  });

                  // Prepare the descriptor
                  {
                     auto& descriptor = common::service::conversation::context().descriptors().reserve( message.correlation);
                     descriptor.route = message.recording;

                     reply.code.result = code::xatmi::ok;

                  }
                  send_reply();


                  policy::call::Default policy;

                  service::call( policy, common::service::conversation::context(), message, true);
               }
               catch( ...)
               {
                  common::exception::handle();
               }

            }


         } // handle
      } // server
   } // common
} // casual
