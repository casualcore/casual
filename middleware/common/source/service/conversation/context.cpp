//!
//! casual 
//!

#include "common/service/conversation/context.h"
#include "common/service/lookup.h"

#include "common/log.h"
#include "common/buffer/transport.h"

#include "common/message/conversation.h"

#include "common/communication/ipc.h"


namespace casual
{

   namespace common
   {
      namespace service
      {
         namespace conversation
         {

            namespace local
            {
               namespace
               {
                  namespace prepare
                  {

                     auto message(
                           State& state,
                           platform::time::point::type& start,
                           char* data,
                           long size,
                           long flags,
                           const message::service::call::Service& service)
                     {
                        message::conversation::connect::Request message;

                        message.correlation = uuid::make();
                        message.service = service;
                        message.process = process::handle();


                        //
                        // we push the ipc-queue-id that this instance has. This will
                        // be the last node (for the route when the other server communicate
                        // with us, in the "reverse" order).
                        //
                        message.route.nodes.push_back( { communication::ipc::inbound::id()});

                        return message;
                     }

                  } // prepare
               } // <unnamed>
            } // local

            Context& Context::instance()
            {
               static Context singleton;
               return singleton;
            }

            Context::Context() = default;


            descriptor::type Context::connect(
                  const std::string& service,
                  platform::buffer::raw::type data,
                  platform::buffer::raw::size::type size,
                  long flags)
            {
               Trace trace{ "common::service::conversation::connect"};

               service::Lookup lookup{ service};

               log::debug << "service: " << service << " data: @" << static_cast< const void*>( data) << " size: " << size << " flags: " << flags << '\n';


               auto start = platform::time::clock::type::now();

               //
               // Invoke pre-transport buffer modifiers
               //
               buffer::transport::Context::instance().dispatch( data, size, service, buffer::transport::Lifecycle::pre_call);


               auto target = lookup();

               if( target.state == message::service::lookup::Reply::State::absent)
               {
                  throw common::exception::xatmi::service::no::Entry( service);
               }

               //
               // The service exists. Take care of reserving descriptor and determine timeout
               //
               auto message = local::prepare::message( m_state, start, data, size, flags, target.service);


               return 0;
            }

         } // conversation

      } // service
   } // common

} // casual
