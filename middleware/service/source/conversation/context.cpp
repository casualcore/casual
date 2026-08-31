//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "service/conversation/context.h"
#include "service/conversation/state.h"
#include "service/lookup.h"

#include "transaction/context.h"

#include "common/log.h"
#include "common/buffer/transport.h"
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
   namespace service::conversation
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
                  if( common::flag::contains( flags, decltype( flags)::receive_only))
                     return state::descriptor::Value::Duplex::receive;
                  else
                     return state::descriptor::Value::Duplex::send;
               }

               template< typename E = state::descriptor::Value::Duplex>
               auto invert( state::descriptor::Value::Duplex duplex)
               {
                  using Duplex = decltype( duplex);
                  switch( duplex)
                  {
                     case Duplex::receive: return E::send;
                     case Duplex::send: return E::receive;
                     default: common::code::raise::error( common::code::casual::invalid_semantics, "duplex: ", duplex);
                  }
               }
            } // duplex

            namespace validate
            {
               void flags( connect::Flag flags)
               {
                  constexpr auto duplex = connect::Flag::send_only | connect::Flag::receive_only;

                  if( common::flag::count( duplex & flags) != 1)
                     common::code::raise::error( common::code::xatmi::argument, "send or receive intention must be provided - flags: ", flags);
               }

               void send( const state::descriptor::Value& value)
               {
                  if( value.duplex != decltype( value.duplex)::send)
                     common::code::raise::error( common::code::xatmi::protocol, "caller has not the control of the conversation");
               }

               void receive( const state::descriptor::Value& value)
               {
                  if( value.duplex != decltype( value.duplex)::receive)
                     common::code::raise::error( common::code::xatmi::protocol, "caller has not the control of the conversation");
               }

               void disconnect( const state::descriptor::Value& value)
               {
                  if( ! value.initiator)
                     common::code::raise::error( common::code::xatmi::descriptor, "caller has not the control of the conversation");
               }

            } // validate

            namespace check
            {
               void disconnect( const state::descriptor::Value& value)
               {
                  common::message::conversation::Disconnect disconnect;

                  if( common::communication::device::non::blocking::receive(
                        common::communication::ipc::inbound::device(),
                        disconnect,
                        value.correlation))
                  {
                     throw exception::conversation::Event{ Event::disconnect};
                  }
               }

               // Helper to check for "pending events". It is intended to
               // be called immediately before doing the "send" in a
               // tpsend. This is to detect the exceptional situations
               // the other end has issused a disconnect (tpsend in
               // subordinate tpsend) or has done a tpreturn (tpsend
               // in initiator).
               // It also handles the case of a pending service_reply
               // that may have been sent by the service manager if/when
               // it detected that the server process terminated while busy. 
               send::Result pending_event( conversation::state::descriptor::Value value)
               {
                  send::Result result; // assumes no return event! User code 0

                  // There can possibly be pending messages that effecitively
                  // terminate the conversation.
                  // * In the "initiator" it can happen that the subordinate has
                  //   done a tpreturn while not in control of the conversation
                  // * In the subordinate there can be a conversation_disconnect
                  //   caused by a disconnect call executed by the initiator.
                  // * In the initiator there can be a service_reply if/when
                  //   the server process has terminated while busy.
                  // Not detecting and handling theese messages MAY be acceptable
                  // but the XATMI spec documents events for reporting events
                  // in the above cases (TPEV_SVCFAIL or TPEV_SVCERR in the
                  // first case, TPEV_DISCONIMM in the second).
                  //
                  // Note that thera are always timing involved. The messages
                  // can "arrive" between us testing for them and when
                  // we do the real "send".
                  // in that case a future call can detect the situation.

                  // We can possibly use the generic dispatch (consume?)
                  // machinery to handle this. A low level soultion is to
                  // do a non-blocking recive for conversation_send or
                  // conversation_disconnect. As we are in control of the
                  // conversation an inbound conversation_send can only be
                  // the result of a subordinate tpreturn, while a
                  // conversation_disconnect is the result of an initiator
                  // disconnect.

                  constexpr auto types = std::array{ 
                     common::message::conversation::callee::Send::type(),
                     common::message::Type::conversation_disconnect,
                     common::message::Type::service_reply};

                  // non-blocking tprecv...
                  auto complete = common::communication::device::non::blocking::next(
                     common::communication::ipc::inbound::device(),
                     types,
                     value.correlation);

                  switch( complete.type())
                  {
                  case common::message::Type::absent_message:
                     // no message, this is the "normal" scenario
                     break;

                  case common::message::Type::conversation_send:
                     {
                        // tpreturn by the subordinate when not in control
                        // Need to generate TPEV_SVCFAIL or TPEV_SVCERR
                        auto message = common::serialize::native::complete< common::message::conversation::callee::Send>( complete);
                        using Result = decltype( message.code.result);
                        switch( message.code.result)
                        {
                           case Result::service_fail:
                              result.event = decltype( result.event)::service_fail;
                              result.user = message.code.user;
                              break;
                           case Result::service_error:
                              result.event = decltype( result.event)::service_error;
                              break;
                           default:
                              // Anything else is abnormal in this situation and should not happen.
                              common::log::error(
                                 common::code::casual::internal_unexpected_value,
                                 "message type: ", message.type(),
                                 " with unexpected code.result: ",
                                 message.code.result,
                                 " handled as service_error");

                              result.event = decltype( result.event)::service_error;
                              break;
                        }
                     break;
                     }
                  case common::message::Type::conversation_disconnect:
                     // do as in local::check::disconnect().
                     throw exception::conversation::Event{ Event::disconnect};
                     break;
                  case common::message::Type::service_reply:
                     // Message from service manager. Server process has
                     // died...
                     // Need to generate TPEV_SVCERR
                     // We do not need the actual message content!
                     // If this occurs it is an unconditional TPEV_SVCERR
                     result.event = decltype( result.event)::service_error;
                     break;
                  default:
                     // should never happen! The next() above only accept
                     // conversation_send, conversation_disconnect and
                     // service_reply.
                     // And absent_message can occur when
                     // neither has arrived. Anything else should be
                     // the result of a coding error/bug.
                     // Log or dump? A defensive strategy is to treat as
                     // a disconnect. (Treat as disconnect in subordinate and
                     // TPEV_SVCERR in initiator?)
                     common::log::error( common::code::casual::internal_unexpected_value,
                        "message type: ", complete.type(),
                        " not expected - action: treated as disconnect");
                     throw exception::conversation::Event{ Event::disconnect};
                     break;
                  }
                  return result;
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
         common::log::debug( "state: ", m_state);

         if( pending())
            common::log::error( common::code::casual::invalid_semantics,  " pending conversations: ", m_state.descriptors.size());
         
      }


      common::strong::conversation::descriptor::id Context::connect(
            const std::string& service,
            common::buffer::payload::Send buffer,
            connect::Flag flags)
      {
         common::Trace trace{ "common::service::conversation::Context::connect"};

         auto create_lookup = []( const auto& service, auto flags, const auto& trid)
         {
            if( common::flag::contains( flags, connect::Flag::no_transaction))
               return service::Lookup{ service, {}};
            else
               return service::Lookup{ service, trid};
         };

         local::validate::flags( flags);

         common::log::debug( "service: ", service, " buffer: ", buffer, " flags: ", flags);

         auto& transaction = casual::transaction::context().current();

         auto lookup = create_lookup( service, flags, transaction.trid);
         

         auto start = platform::time::clock::type::now();

         auto descriptor = m_state.descriptors.reserve(
            // Use the correlation id from the lookup
            // in the connect! Seems as if service manager remembers
            // this correlation id and uses it to send a 
            // "service reply" if/when the server executing the service
            // terminates for some reason. For this to work
            // the conversation receive (and send?) calls need to use
            // the same correlation id to wait for/read responses
            // from the service
            lookup.correlation(),
            common::process::Handle{},
            local::duplex::convert( flags),
            true);

         auto& value = m_state.descriptors.at( descriptor);

         common::log::debug( "descriptor: ", descriptor, ", value: ", value);

         // If some thing goes wrong we unreserve the descriptor
         auto unreserve = common::execute::scope( [&](){ m_state.descriptors.unreserve( descriptor);});

         auto target = service::lookup::reply( std::move( lookup));

         // The service exists. prepare the request
         auto message = local::prepare::message< common::message::conversation::connect::caller::Request>( value, std::move( buffer), common::process::handle());
         {
            // set stuff from lookup-reply (service, span, deadline, etc
            message.update( target);
            message.parent.span = common::execution::context::get().span;
            message.parent.service = common::execution::context::get().service;
            message.duplex = local::duplex::invert( value.duplex);

            if( ! common::flag::contains( flags, connect::Flag::no_transaction) && transaction)
            {
               message.trid = transaction.trid;
               transaction.associate( message.correlation);
            }
         }

         // If something goes wrong (most likely a timeout), we need to send ack to broker in that case, cus the service(instance)
         // will not do it...
         auto send_ack = common::execute::scope( [&]()
         {
            common::message::service::call::ACK ack;

            ack.execution = message.execution;
            ack.metric.execution = message.execution;
            ack.metric.span = message.span;
            ack.metric.service = service;
            ack.metric.parent = message.parent;
            ack.metric.process = common::process::handle();
            ack.metric.trid = message.trid;

            ack.metric.start = start;
            ack.metric.end = platform::time::clock::type::now();

            common::communication::device::blocking::send( common::communication::instance::outbound::service::manager::device(), ack);
         });


         value.process = target.process;

         common::log::debug( "descriptor: ", descriptor, ", value: ", value);

         // connect to the service
         {
            message.service = target.service;

            common::log::debug( "connect - request: ", message);

            auto reply = common::communication::ipc::call( target.process.ipc, message);

            common::log::debug( "connect - reply: ", reply);

         }

         unreserve.release();
         send_ack.release();


         return descriptor;
      }

      send::Result Context::send( common::strong::conversation::descriptor::id descriptor, common::buffer::payload::Send&& buffer, send::Flag flags)
      {
         common::Trace trace{ "common::service::conversation::Context::send"};

         auto& value = m_state.descriptors.at( descriptor);
         common::log::debug( "descriptor: ", descriptor, ", value: ", value);

         local::validate::send( value);

         value.duplex = local::duplex::convert( flags);
         // if an event occurs (TPEV_DISCONIMM, TPEV_SVCFAI or TPEV_SVCERR)
         // the conversation is terminated. The descriptor shall be unreserved.
         auto unreserve = common::execute::scope( [&](){ m_state.descriptors.unreserve( descriptor);});

         local::check::disconnect( value);

         // In this situation we may have some odd situations. We are in
         // control of the conversation (checked by
         // local::validate::send( value) above) and about to do a "send".
         // There can possibly be pending messages that effectively
         // terminate the conversation.
         // * In the "initiator" it can happen that the subordinate has
         //   done a tpreturn while not in control of the conversation
         // * in the subordinate there can be a conversation_disconnect
         //   caused by a disconnect call executed by the initiator.
         // Not detecting and handling these messages MAY be acceptable,
         // but the XATMI spec documents events for reporting events
         // in the above cases. TPEV_SVCFAIL or TPEV_SVCERR in the
         // first case, TPEV_DISCONIMM in the second.
         //
         // Note that thera are always timing involved. The messages
         // can "arrive" between us testing for them and the "send"
         // issued below. The situation will in that case be detected
         // on a future call (potentially many calls into the future
         // if many send are done in sequence). This is what makes it
         // "maybe acceptable" to not detect the sitiation. But for
         // early detection, we should check for this.

         // We can possibly use the generic dispatch (consume?)
         // machinery to handle this. A low level solution is to
         // do a non-blocking recive for conversation_send or
         // conversation_disconnect. As we are in control of the
         // conversation an inbound conversation_send can only be
         // the result of a subordinate tpreturn, while a
         // conversation_disconnect is the result of an initiator
         // disconnect.
         // This low level impementation is in check::pending_event()
         //common::Flag< Event> result_event = local::check::pending_event( value);
         send::Result result = local::check::pending_event( value);

         if( common::flag::empty( result.event))
         {
            // normal case! No pending event so prepare and send data
            auto message = local::prepare::message< common::message::conversation::caller::Send>( value, std::move( buffer));

            message.duplex = local::duplex::invert<decltype( message.duplex)>( value.duplex);

            common::communication::device::blocking::send( value.process.ipc, message);

            unreserve.release(); // the conversation remains, so descriptor
                                 // shall not be unreserved
         }

         return result;

      }

      receive::Result Context::receive( common::strong::conversation::descriptor::id descriptor, receive::Flag flags)
      {
         common::Trace trace{ "common::service::conversation::Context::receive"};

         auto& value = m_state.descriptors.at( descriptor);
         common::log::debug( "descriptor: ", descriptor, ", value: ", value);

         local::validate::receive( value);

         auto unreserve = common::execute::scope( [&](){ m_state.descriptors.unreserve( descriptor);});

         local::check::disconnect( value);

         // There are three possible messages in this situation:
         //  common::message::conversation::calle::send
         //  common::message::Type::conversation_disconnect
         //  common::message::Type::service_reply
         // This means we must use common::communication::device methods that
         // allows this. The usual
         //   common::communication::device[::non]::blocking(receive(communication::ipc::inbound::device(),
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
         // For now,  I am using a simple approach (but not elegant!) and use
         // primitives provided by Inbound<typename Connector>.

         // Normally we will get a conversation_send message and
         // the content of that message is needed. We do NOT need
         // the deserialized content of a possible (but unusual)
         // disconnect_message. The existence of the message is enough.
         // The same is true for service_reply also. This message
         // comes from the service manager, and occurs when the service
         // manager has detected that the server process terminated
         // and was busy (with an active service).
         //
         // To get a service_reply from a conversational service is
         // a bit odd. A cleaner design might have been to have a
         // specific common::message::Type for notificationa about service
         // errors. Now the conversational case relies on the logic
         // and messaages used for "tpcall"  

         auto receive_message = [&unreserve]( auto& correlation, auto& flags)
         {
            // types of interest...
            constexpr static auto types = common::array::make(
                  common::message::conversation::callee::Send::type(),
                  common::message::Type::conversation_disconnect,
                  common::message::Type::service_reply);

            auto receive_complete = []( auto& correlation, auto& flags)
            {
               if( common::flag::contains( flags, receive::Flag::no_block))
               {  
                  return common::communication::device::non::blocking::next(
                     common::communication::ipc::inbound::device(),
                     types,
                     correlation);
               }

               return common::communication::device::blocking::next(
                  common::communication::ipc::inbound::device(),
                  types,
                  correlation);
            };

            auto complete = receive_complete( correlation, flags);

            if( ! complete)
            {
               unreserve.release();
               common::code::raise::error( common::code::xatmi::no_message);
            }

            switch( complete.type())
            {
               case common::message::Type::conversation_send:
               {
                  // happy path
                  common::message::conversation::callee::Send message;
                  common::serialize::native::complete( complete, message);
                  return message;
               }
               case common::message::Type::conversation_disconnect:
               {
                  // "error" path
                  // do as in local::check::disconnect().
                  throw exception::conversation::Event{ Event::disconnect};
               }
               case common::message::Type::service_reply:
               {
                  // "error" path
                  // It is somewhat odd to get a service_reply 
                  // in a conversational context! This occurs if/when
                  // a server processing a (subordinate) conversational
                  // service diees. The service manager discovers this
                  // and sends a service_reply. This is the expected
                  // message in a "tpcall" context... 
                  throw exception::conversation::Event{ Event::service_error};
               }
               default:
               {
                  // fatal path - not possible
                  casual::terminate( "unexpected message type consumed: ", complete);
               }
            }
         };

         auto message = receive_message( value.correlation, flags);
         common::log::debug( "message: ", message);

         receive::Result result;
         result.buffer = std::move( message.buffer);

         // check if other side has done a tpreturn
         // This should/can only happen when we are "initiator"
         // of the conversation. A tpreturn in the "initiator" would
         // terminate that service (assuming it is part of an invoked
         // service), and not the conversation. Indirectly
         // it terminates the conversation but that is a consequence
         // of the termination of the initiator!
         //
         // A tpreturn can supply a user return code that is passed
         // on to the caller of tprecv. The user return code is only passed
         // on SVCSUCC and SVCFAIL. It is not passed with "SVCERR".
         // Note that as this code is called in the context of a tprecv,
         // the other end is likely to be in control of the conversation
         // when/if it calls tpreturn. This means that a user return code
         // and possibly a data buffer can be sent by the tpreturn.
         // There can be exotic timing cases where the tpreturn is
         // called by a service that is NOT in control of the conversation,
         // but when the "initiator" receives the message sent by the tpretun
         // the initator has transferrred the control of the conversation.
         //
         // initiator               service
         //  tpsend
         //                         tprecv
         //  tpsend TPRECVONLY
         //                         tpreturn TPSVCFAIL and no data, but user code
         //  tprecv
         //                         (message sent with TPRECVONLY is discarded)
         //
         // If the timing is slightly different and the initator has got the
         // "tpreturn message" when the call to tpsend with TPRECVONLY is issued,
         // then that call can return TPEV_SVCFAIL. The tpsend is documented as 
         // setting the global "tpurcode" in this case. This implies that the user
         // return code is to be passed even for tpreturn when not in control
         // of the session (as long as no data buffer is passed to tpreturn).     

         if( message.duplex == decltype( message.duplex)::terminated)
         {
            using Result = decltype( message.code.result);
            switch( message.code.result)
            {
               case Result::ok:
                  result.event = decltype( result.event)::service_success;
                  result.user = message.code.user;
               break;
               case Result::service_fail:
                  result.event = decltype( result.event)::service_fail;
                  result.user = message.code.user;
                  break;
               default:
                  result.event = decltype( result.event)::service_error;
                  result.user = 0; // a predictable user return code...
                  break;
            }
            
            // we're done with this conversation,
            // descriptor will be unreserved
            return result;
         }
         
         if( message.duplex == decltype( message.duplex)::send)
         {
            value.duplex = decltype( value.duplex)::send;
            result.event = decltype( result.event)::send_only;
         }

         common::log::debug( "value: ", value);
         common::log::debug( "result: ", result);

         unreserve.release(); // keep descriptor for conversation

         return result;
      }

      void Context::disconnect( common::strong::conversation::descriptor::id handle)
      {
         common::Trace trace{ "common::service::conversation::Context::disconnect"};

         auto& value = m_state.descriptors.at( handle);
         common::log::debug( "descriptor: ", handle, ", value: ", value);
         

         local::validate::disconnect( value);

         auto unreserve = common::execute::scope( [&](){ m_state.descriptors.unreserve( handle);});

         {
            auto message = local::prepare::message< common::message::conversation::Disconnect>( value);
            common::communication::device::blocking::send( value.process.ipc, message);
         }

         // If we're not in control we "need" to discard the possible incoming message
         if( value.duplex == decltype( value.duplex)::receive)
            common::communication::ipc::inbound::device().discard( value.correlation);

      }

      bool Context::pending() const
      {
         return ! m_state.descriptors.empty();
      }

   } // service::conversation
} // casual
