//!
//! casual 
//!

#include "common/server/handle/conversation.h"
#include "common/server/handle/service.h"
#include "common/server/handle/policy.h"

#include "common/service/conversation/context.h"


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
               Trace trace{ "server::Conversation::operator()"};

               log::debug << "message: " << message << '\n';

               try
               {
                  auto reply = message::reverse::type( message);
                  // We set the worst we got until proven otherwise.
                  reply.status = TPEPROTO;

                  auto send_reply = scope::execute( [&](){
                     reply.process = process::handle();
                     reply.route = message.recording;
                  });

                  //
                  // Prepare the descriptor
                  //
                  {
                     auto& descriptor = common::service::conversation::context().descriptors().reserve( message.correlation);
                     descriptor.route = message.recording;

                     reply.status = 0;

                  }
                  send_reply();


                  policy::call::Default policy;

                  service::call( policy, common::service::conversation::context(), message);
               }
               catch( ...)
               {
                  error::handler();
               }

            }


         } // handle
      } // server
   } // common
} // casual
