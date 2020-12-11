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

               try
               {
                  auto reply = message::reverse::type( message);
                  // We set the worst we got until proven otherwise.
                  reply.code.result = code::xatmi::protocol;
                  reply.process = process::handle();
                  //
                  // The following two lines are suspect!
                  // At least the first of them.
                  // What should reply.recording and reply.route contain?
                  // message.recording contains the
                  // caller ipc (in the case of local caller and callee).
                  // Conceptually, my understanding is that message.recording
                  // contains the path taken by the connect message 
                  // (where the message comes from). The callee's own ipc
                  // is "known" already, and this may be why it is not
                  // included at the end.
                  // If reply.route is the path the reply shall take, then it
                  // ought to be the reverse of message.recording. The reverse
                  // of a one entry vector is the the same, so reversing
                  // the list makes no difference in that case.
                  // I checked what happens in send_reply:
                  //    auto node = reply.route.next() 
                  // returns the LAST entry in reply.route.nodes, and
                  // removes it. Means that reply.route.nodes is used "backwards"
                  // (as a "stack" with last entry at the top)
                  // so reply.route.nodes need no be reversed. I know that the
                  // response to connect gets to the caller (at least for local)
                  // so reply.route = message.recording is reasonable.
                  //   
                  // But what about reply.recording? At least in the case
                  // local conversation reusing message.recording does not work.
                  // The "route" set in the caller descriptor will not be right.
                  // The first entry will be the caller ipc, and the callee ipc
                  // will be missing. A possible alternative is to 
                  // omit the firsty entry from message.recording, and to add
                  // our own inound ipc at the end.
                  // How is the calle descriptor.route used in a tpsend?
                  // <answer: callee (tp)send uses back() of descriptor.route.nodes
                  // as the first hop towards callee. >
                  //
                  // As a test (local only) just setting our own inbound
                  // ipc should work.
                  // To work with gateways (multiple entries in route.nodes)
                  // the connect message.recording need to be modified to
                  // be usable as the caller descriptor.route (before being
                  // sent back to the caller in the connect response).
                  // * add callee inbound inbound to end
                  // * reverse order
                  // * remove last entry (removes caller ipc)

                  //reply.recording = message.recording;
                  //reply.route = message.recording;
                  auto nodes = message.recording.nodes;
                  nodes.emplace_back( communication::ipc::inbound::ipc());
                  std::reverse(nodes.begin(), nodes.end());
                  nodes.pop_back();  
                  reply.recording.nodes = std::move(nodes);
                  reply.route = message.recording;;


                  auto send_reply = execute::scope( [&]()
                  {
                     auto node = reply.route.next();
                     communication::device::blocking::send( node.address, reply);
                  });

                  // Prepare the descriptor
                  {
                     // moved descriptor setup to common/source/server/gandle/service.cpp.
                     // creating it here AND there with reserve caused two descriptors
                     // to be allocated.
                     //auto& descriptor = common::service::conversation::context().descriptors().reserve( message.correlation);
                     //descriptor.route = message.recording;
                     //if (message.flags.exist(casual::common::flag::service::conversation::connect::Flag::receive_only))
                     //{ // caller specified receive only, so we have control of conversation
                     // descriptor.duplex = common::service::conversation::state::descriptor::Information::Duplex::send;
                     //}
                     reply.code.result = code::xatmi::ok;

                  }
                  send_reply();


                  policy::call::Default policy;

                  service::call( policy, common::service::conversation::context(), message, true);
               }
               catch( ...)
               {
                  log::line( log::category::error, exception::capture(), " conversation connect failed");
                  log::line( log::category::verbose::error, "message: ", message);
               }

            }


         } // handle
      } // server
   } // common
} // casual
