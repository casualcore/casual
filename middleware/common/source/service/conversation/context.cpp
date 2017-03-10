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

                     auto connect(
                           State& state,
                           platform::time::point::type& start,
                           common::buffer::payload::Send buffer,
                           common::Flags< connect::Flag> flags,
                           const message::service::call::Service& service)
                     {
                        message::conversation::connect::caller::Request message{ std::move( buffer)};

                        message.correlation = uuid::make();
                        message.service = service;
                        message.process = process::handle();


                        auto& descriptor = state.descriptors.reserve( message.correlation);

                        message.descriptor = descriptor.descriptor;

                        //
                        // we push the ipc-queue-id that this instance has. This will
                        // be the last node (for the route when the other server communicate
                        // with us, in the "reverse" order).
                        //
                        message.recording.nodes.push_back( { communication::ipc::inbound::id()});

                        return message;
                     }

                     message::conversation::send::caller::Request send(
                           const State::descriptor_type& descriptor,
                           common::buffer::payload::Send&& buffer,
                           send::Flags flags)
                     {
                        message::conversation::send::caller::Request message{ std::move( buffer)};

                        message.correlation = descriptor.correlation;
                        message.process = process::handle();
                        message.route = descriptor.route;
                        message.execution = execution::id();


                        return message;
                     }
                  } // prepare

                  namespace validate
                  {
                     void send( const state::descriptor::Information& descriptor)
                     {
                        Trace trace{ "common::service::conversation::local::validate::send"};

                        if( descriptor.duplex != state::descriptor::Information::Duplex::send)
                        {
                           throw exception::xatmi::Protocoll{ "caller has not the control of the conversation"};
                        }


                     }

                     void receive( const state::descriptor::Information& descriptor)
                     {
                        Trace trace{ "common::service::conversation::local::validate::send"};

                        if( descriptor.duplex != state::descriptor::Information::Duplex::receive)
                        {
                           throw exception::xatmi::Protocoll{ "caller has not the control of the conversation"};
                        }


                     }

                  } // validate

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
                  common::buffer::payload::Send buffer,
                  connect::Flags flags)
            {
               Trace trace{ "common::service::conversation::connect"};

               service::Lookup lookup{ service};

               log::debug << "service: " << service << " buffer: " << buffer << " flags: " << flags << '\n';


               auto start = platform::time::clock::type::now();

               auto& descriptor = m_state.descriptors.reserve( uuid::make());

               //
               // TODO: Invoke pre-transport buffer modifiers
               //
               //buffer::transport::Context::instance().dispatch( data, size, service, buffer::transport::Lifecycle::pre_call);


               auto target = lookup();

               if( target.state == message::service::lookup::Reply::State::absent)
               {
                  throw common::exception::xatmi::service::no::Entry( service);
               }

               //
               // The service exists. Take care of reserving descriptor and determine timeout
               //
               auto message = local::prepare::connect( m_state, start, std::move( buffer), flags, target.service);

               //
               // If some thing goes wrong we unreserve the descriptor
               //
               auto unreserve = common::scope::execute( [&](){ m_state.descriptors.unreserve( message.descriptor);});

               //
               // If something goes wrong (most likely a timeout), we need to send ack to broker in that case, cus the service(instance)
               // will not do it...
               //
               auto send_ack = common::scope::execute( [&]()
                  {
                     message::service::call::ACK ack;
                     ack.process = target.process;
                     ack.service = target.service.name;
                     communication::ipc::blocking::send( communication::ipc::broker::device(), ack);
                  });

               if( target.state == message::service::lookup::Reply::State::busy)
               {
                  //
                  // We wait for an instance to become idle.
                  //
                  target = lookup();
               }

               //
               // connect to the service
               //
               {
                  message.service = target.service;

                  log::debug << "connect - message: " << message << std::endl;

                  auto reply = communication::ipc::call( target.process.queue, message);

                  descriptor.route = std::move( reply.route);
               }

               unreserve.release();
               send_ack.release();


               return 0;
            }

            common::Flags< Event> Context::send( descriptor::type handle, common::buffer::payload::Send&& buffer, common::Flags< send::Flag> flags)
            {
               Trace trace{ "common::service::conversation::send"};

               auto& descriptor = m_state.descriptors.get( handle);

               local::validate::send( descriptor);

               auto message = local::prepare::send( descriptor, std::move( buffer), flags);

               //
               // Check if the user wants to transfer the control of the conversation.
               //
               if( flags & send::Flag::receive_only)
               {
                  descriptor.duplex = decltype( descriptor.duplex)::receive;
                  message.events = { Event::send_only};
               }

               auto node = message.route.next();

               auto reply = communication::ipc::call( node.address, message);

               return reply.events;

            }

            receive::Result Context::receive( descriptor::type handle, common::Flags< receive::Flag> flags)
            {
               Trace trace{ "common::service::conversation::receive"};

               auto& descriptor = m_state.descriptors.get( handle);

               local::validate::receive( descriptor);

               message::conversation::send::callee::Request message;

               if( ! communication::ipc::receive::message(
                     communication::ipc::inbound::device(),
                     message,
                     flags & receive::Flag::no_block ?
                           communication::ipc::receive::Flag::non_blocking : communication::ipc::receive::Flag::blocking,
                     descriptor.correlation))
               {
                  throw common::exception::xatmi::no::Message();
               }

               log::debug << "message: " << message << '\n';

               receive::Result result;
               result.buffer = std::move( message.buffer);
               result.event = message.events;

               constexpr Events termination_events{
                  Event::disconnect, Event::service_error, Event::service_fail, Event::service_success};

               if( result.event & termination_events)
               {
                  //
                  // Other side has terminated the conversation
                  //


               }
               else
               {
                  //
                  // Send reply
                  //
                  {
                     auto reply = message::reverse::type( message);


                     reply.process = process::handle();
                     reply.route = descriptor.route;

                     log::debug << "reply: " << reply << '\n';

                     auto node = message.route.next();

                     communication::ipc::blocking::send( node.address, message);
                  }
               }


               return result;

            }

            bool Context::pending() const
            {
               return false;
            }

         } // conversation

      } // service
   } // common

} // casual
