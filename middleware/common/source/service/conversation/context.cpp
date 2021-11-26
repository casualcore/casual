//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/service/conversation/context.h"
#include "common/service/lookup.h"

#include "common/log.h"
#include "common/buffer/transport.h"
#include "common/transaction/context.h"
#include "common/execute.h"
#include "common/array.h"

#include "common/message/conversation.h"

#include "common/code/raise.h"
#include "common/code/xatmi.h"

#include "common/communication/ipc.h"
#include "common/communication/instance.h"

#include "casual/assert.h"

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
                     template< typename Message, typename... Args>
                     Message message( const state::descriptor::Value& value, Args&&... args)
                     {
                        Message result{ std::forward< Args>( args)...};
                        result.correlation = value.correlation;
                        return result;
                     }
                  } // prepare


                  namespace duplex
                  {
                     template< typename F>
                     auto convert( F flags)
                     {
                        if( flags.exist( decltype( flags.type())::receive_only))
                           return state::descriptor::Value::Duplex::receive;
                        else
                           return state::descriptor::Value::Duplex::send;
                     }

                     auto invert( state::descriptor::Value::Duplex duplex)
                     {
                        using Duplex = decltype( duplex);
                        switch( duplex)
                        {
                           case Duplex::receive: return Duplex::send;
                           case Duplex::send: return Duplex::receive;
                           default: code::raise::error( code::casual::invalid_semantics, "duplex: ", duplex);
                        }
                     }
                  } // duplex

                  namespace validate
                  {
                     void flags( connect::Flags flags)
                     {
                        constexpr connect::Flags duplex{ connect::Flag::send_only, connect::Flag::receive_only};

                        if( ( flags & duplex) == duplex || ! ( flags & duplex))
                           code::raise::error( code::xatmi::argument, "send or receive intention must be provided - flags: ", flags);
                     }

                     void send( const state::descriptor::Value& value)
                     {
                        if( value.duplex != decltype( value.duplex)::send)
                           code::raise::error( code::xatmi::protocol, "caller has not the control of the conversation");
                     }

                     void receive( const state::descriptor::Value& value)
                     {
                        if( value.duplex != decltype( value.duplex)::receive)
                           code::raise::error( code::xatmi::protocol, "caller has not the control of the conversation");
                     }

                     void disconnect( const state::descriptor::Value& value)
                     {
                        if( ! value.initiator)
                           code::raise::error( code::xatmi::descriptor, "caller has not the control of the conversation");
                     }

                  } // validate

                  namespace check
                  {
                     void disconnect( const state::descriptor::Value& value)
                     {
                        message::conversation::Disconnect disconnect;

                        if( communication::device::non::blocking::receive(
                              communication::ipc::inbound::device(),
                              disconnect,
                              value.correlation))
                        {
                           throw exception::conversation::Event{ Event::disconnect};
                        }
                     }

                  } // check

               } // <unnamed>
            } // local

            Context& Context::instance()
            {
               static Context singleton;
               return singleton;
            }

            Context::Context() = default;

            Context::~Context()
            {
               log::line( verbose::log, "state: ", m_state);

               if( pending())
                  log::line( log::category::error, code::casual::invalid_semantics,  " pending conversations: ", m_state.descriptors.size());
               
            }


            strong::conversation::descriptor::id Context::connect(
                  const std::string& service,
                  common::buffer::payload::Send buffer,
                  connect::Flags flags)
            {
               Trace trace{ "common::service::conversation::Context::connect"};

               local::validate::flags( flags);

               service::Lookup lookup{ service};
               log::line( log::debug, "service: ", service, " buffer: ", buffer, " flags: ", flags);


               auto start = platform::time::clock::type::now();

               auto descriptor = m_state.descriptors.reserve( 
                  strong::correlation::id{ uuid::make()},
                  process::Handle{},
                  local::duplex::convert( flags),
                  true);

               auto& value = m_state.descriptors.at( descriptor);

               log::line( log::debug, "descriptor: ", descriptor, ", value: ", value);

               // If some thing goes wrong we unreserve the descriptor
               auto unreserve = common::execute::scope( [&](){ m_state.descriptors.unreserve( descriptor);});

               auto target = lookup();

               // The service exists. prepare the request
               auto message = local::prepare::message< message::conversation::connect::caller::Request>( value, std::move( buffer), process::handle());
               {
                  message.service = service;
                  message.parent = common::execution::service::name();
                  message.duplex = local::duplex::invert( value.duplex);

                  auto& transaction = common::transaction::context().current();

                  if( ! flags.exist( connect::Flag::no_transaction) && transaction)
                  {
                     message.trid = transaction.trid;
                     transaction.associate( message.correlation);
                  }
               }

               // If something goes wrong (most likely a timeout), we need to send ack to broker in that case, cus the service(instance)
               // will not do it...
               auto send_ack = common::execute::scope( [&]()
               {
                  message::service::call::ACK ack;

                  ack.execution = message.execution;
                  ack.metric.execution = message.execution;
                  ack.metric.service = service;
                  ack.metric.parent = message.parent;
                  ack.metric.process = common::process::handle();
                  ack.metric.trid = message.trid;

                  ack.metric.start = start;
                  ack.metric.end = platform::time::clock::type::now();

                  communication::device::blocking::send( communication::instance::outbound::service::manager::device(), ack);
               });

               
               if( target.busy())
               {
                  // We wait for an instance to become idle.
                  target = lookup();
               }

               value.process = target.process;

               log::line( verbose::log, "descriptor: ", descriptor, ", value: ", value);

               // connect to the service
               {
                  message.service = target.service;

                  log::line( log::debug, "connect - request: ", message);

                  auto reply = communication::ipc::call( target.process.ipc, message);

                  log::line( log::debug, "connect - reply: ", reply);

               }

               unreserve.release();
               send_ack.release();


               return descriptor;
            }

            common::Flags< Event> Context::send( strong::conversation::descriptor::id descriptor, common::buffer::payload::Send&& buffer, common::Flags< send::Flag> flags)
            {
               Trace trace{ "common::service::conversation::Context::send"};

               auto& value = m_state.descriptors.at( descriptor);
               log::line( verbose::log, "descriptor: ", descriptor, ", value: ", value);

               local::validate::send( value);

               value.duplex = local::duplex::convert( flags);

               auto unreserve = common::execute::scope( [&](){ m_state.descriptors.unreserve( descriptor);});

               local::check::disconnect( value);

               auto message = local::prepare::message< message::conversation::caller::Send>( value, std::move( buffer));
               message.duplex = local::duplex::invert( value.duplex);

               communication::device::blocking::send( value.process.ipc, message);

               unreserve.release();

               return {};

            }

            receive::Result Context::receive( strong::conversation::descriptor::id descriptor, common::Flags< receive::Flag> flags)
            {
               Trace trace{ "common::service::conversation::Context::receive"};

               auto& value = m_state.descriptors.at( descriptor);
               log::line( verbose::log, "descriptor: ", descriptor, ", value: ", value);

               local::validate::receive( value);

               auto unreserve = common::execute::scope( [&](){ m_state.descriptors.unreserve( descriptor);});

               local::check::disconnect( value);

               // There are two possible messages in this situation:
               //  message::conversation::calle::send
               //  message::conversation::disconnect
               // This means we must use communication::device methods that
               // allows this. The usual
               //   communication::device[::non]::blocking(receive(communication::ipc::inbound::device(),
               //                                          message,
               //                                          value.correlation))
               // only accepts messages matching the "message" argument ((message.type())
               // we can possibly use a scheme similar to the "dispatch" used as
               // the server message pump and set up a scheme where handlers can
               // be registered and dispatched to. This is not a message
               // pump as we ony want a "one-shot", so we can not directly use
               // that (unless it provides for such a mode). It is also a
               // bit of overkill to set up all that machinery.
               //
               // For now,  I am using a more simpler approach and use
               // primitives provided by Inbound<typename Connector>.

               // Normally we will get a conversation_send message and
               // the content of that message is needed. We do NOT need
               // the deserialized content of a possible (but unusual)
               // disconnect_message.

               auto receive_message = [&unreserve]( auto& correlation, auto& flags)
               {
                  // types of interest...
                  constexpr static auto types = common::array::make( message::conversation::callee::Send::type(), message::Type::conversation_disconnect);

                  auto receive_complete = []( auto& correlation, auto& flags)
                  {
                     if( flags.exist( receive::Flag::no_block))
                     {  
                        return communication::device::non::blocking::next(
                           communication::ipc::inbound::device(),
                           types,
                           correlation);
                     }

                     return communication::device::blocking::next(
                        communication::ipc::inbound::device(),
                        types,
                        correlation);
                  };

                  auto complete = receive_complete( correlation, flags);

                  if( ! complete)
                  {
                     unreserve.release();
                     code::raise::error( code::xatmi::no_message);
                  }

                  switch( complete.type())
                  {
                     case message::Type::conversation_send:
                     {
                        // happy path
                        message::conversation::callee::Send message;
                        serialize::native::complete( complete, message);
                        return message;
                     }
                     case message::Type::conversation_disconnect:
                     {
                        // "error" path
                        // do as in local::check::disconnect().
                        throw exception::conversation::Event{ Event::disconnect};
                     }
                     default:
                     {
                        // fatal path - not possible
                        casual::terminate( "unexpected message type consumed: ", complete);
                     }
                  }
               };

               auto message = receive_message( value.correlation, flags);
               log::line( verbose::log, "message: ", message);

               receive::Result result;
               result.buffer = std::move( message.buffer);

               // check if other side has done a tpreturn
               // This should/can only happen when we are "initiator"
               // of the conversation. A tpreturn in the "initiator"
               // terminates that service (assuming it is part of an invoked
               // service), and not the conversation. Indirectly
               // it terminates the conversation but that is a consequence
               // of the termination of the initiator

               using Result = decltype( message.code.result);
               if( message.code.result != Result::absent)
               {
                  switch( message.code.result)
                  {
                     case Result::ok: result.event = decltype( result.event.type())::service_success; break;
                     case Result::service_fail: result.event = decltype( result.event.type())::service_fail; break;
                     default: result.event = decltype( result.event.type())::service_error; break;
                  }
                  
                  // we're done with this conversation,
                  // descriptor will be unreserved
                  return result;
               }
               
               if( message.duplex == decltype( message.duplex)::send)
               {
                  value.duplex = message.duplex;
                  result.event = decltype( result.event.type())::send_only;
               }

               log::line( verbose::log, "value: ", value);
               log::line( verbose::log, "result: ", result);

               unreserve.release(); // keep descriptor for conversation

               return result;
            }

            void Context::disconnect( strong::conversation::descriptor::id handle)
            {
               Trace trace{ "common::service::conversation::Context::disconnect"};

               auto& value = m_state.descriptors.at( handle);
               log::line( verbose::log, "descriptor: ", handle, ", value: ", value);
               

               local::validate::disconnect( value);

               auto unreserve = common::execute::scope( [&](){ m_state.descriptors.unreserve( handle);});

               {
                  auto message = local::prepare::message< message::conversation::Disconnect>( value);
                  communication::device::blocking::send( value.process.ipc, message);
               }

               // If we're not in control we "need" to discard the possible incoming message
               if( value.duplex == decltype( value.duplex)::receive)
                  communication::ipc::inbound::device().discard( value.correlation);

            }

            bool Context::pending() const
            {
               return ! m_state.descriptors.empty();
            }

         } // conversation

      } // service
   } // common

} // casual
