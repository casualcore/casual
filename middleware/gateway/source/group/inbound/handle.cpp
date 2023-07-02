//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/group/inbound/handle.h"
#include "gateway/group/ipc.h"

#include "gateway/message.h"
#include "gateway/common.h"

#include "domain/discovery/api.h"

#include "common/communication/device.h"
#include "common/communication/instance.h"
#include "common/message/dispatch/handle.h"
#include "common/message/internal.h"
#include "common/event/listen.h"

#include "casual/assert.h"

namespace casual
{
   using namespace common;

   namespace gateway::group::inbound::handle
   {   
      namespace local
      {
         namespace
         {
            namespace tcp
            {
               template< typename M>
               strong::correlation::id send( State& state, strong::file::descriptor::id descriptor, M&& message)
               {
                  if( auto connection = state.external.connection( descriptor))
                  {
                     try
                     {
                        return connection->send( state.directive, std::forward< M>( message));
                     }
                     catch( ...)
                     {
                        const auto error = exception::capture();

                        auto lost = connection::lost( state, descriptor);

                        if( error.code() != code::casual::communication_unavailable)
                           log::line( log::category::error, error, " send failed to remote: ", lost.remote, " - action: remove connection");

                        // we 'lost' the connection in some way - we put a connection::Lost on our own ipc-device, and handle it
                        // later (and differently depending on if we're 'regular' or 'reversed')
                        communication::ipc::inbound::device().push( std::move( lost));
                     }
                  }
                  else
                  {
                     log::line( log::category::error, code::casual::internal_correlation, " tcp::send -  failed to correlate descriptor: ", descriptor, " for message type: ", message.type());
                     log::line( log::category::verbose::error, "state: ", state);
                  }

                  return {};
               }

               template< typename M>
               strong::file::descriptor::id send( State& state, M&& message)
               {
                  if( auto descriptor = state.consume( message.correlation))
                  {
                     if( tcp::send( state, descriptor, std::forward< M>( message)))
                        return descriptor;
                  }
               
                  log::line( log::category::error, code::casual::communication_unavailable, " connection absent when trying to send reply - ", message.type());
                  log::line( log::category::verbose::error, "state: ", state);
               
                  return {};
               }
               
            } // tcp

            namespace internal
            {
               template< typename Message>
               auto basic_forward( State& state)
               {
                  return [&state]( Message& message)
                  {
                     Trace trace{ "gateway::group::inbound::handle::local::basic_forward"};
                     common::log::line( verbose::log, "forward message: ", message);

                     tcp::send( state, message);
                  };
               }

               namespace service
               {
                  namespace lookup
                  {
                     namespace detail
                     {
                        template< typename R, typename E>
                        void reply( State& state, R&& request, common::message::service::lookup::Reply& lookup, E send_error)
                        {
                           
                           switch( lookup.state)
                           {
                              using Enum = decltype( lookup.state);
                              case Enum::idle:
                              {
                                 state.multiplex.send( lookup.process.ipc, std::forward< R>( request), [ &state, send_error]( auto& destination, auto& message)
                                 {
                                    log::line( common::log::category::error, common::code::xatmi::service_error, " destination: ", destination, " has been 'removed' during interdomain call - action: reply with: ", common::code::xatmi::service_error);
                                    send_error( state, message.correlation(), common::code::xatmi::service_error);
                                 });
                                 break;
                              }
                              case Enum::absent:
                              {
                                 log::line( common::log::category::error, common::code::xatmi::no_entry, " service: ", lookup.service, " is not handled by this domain (any more) - action: reply with: ", common::code::xatmi::no_entry);
                                 send_error( state, lookup.correlation, common::code::xatmi::no_entry);
                                 break;
                              }
                              default:
                              {
                                 log::line( common::log::category::error, common::code::xatmi::system, " unexpected state on lookup reply: ", lookup, " - action: reply with: ", common::code::xatmi::service_error);
                                 send_error( state, lookup.correlation, common::code::xatmi::system);
                                 break;
                              }
                           }

                        }
                     } // detail

                     auto reply( State& state)
                     {
                        return [&state]( common::message::service::lookup::Reply& lookup)
                        {
                           Trace trace{ "gateway::group::inbound::handle::local::internal::call::lookup::reply"};
                           common::log::line( verbose::log, "message: ", lookup);

                           auto request = state.pending.requests.consume( lookup.correlation, lookup);

                           switch( common::message::type( request))
                           {
                              using Type = decltype( common::message::type( request));

                              case Type::service_call:

                                 detail::reply( state, std::move( request), lookup, []( auto& state, auto& correlation, auto code)
                                 {
                                    common::message::service::call::Reply reply;
                                    reply.correlation = correlation;
                                    reply.code.result = code;
                                    tcp::send( state, reply);
                                 });

                                 break;

                              case Type::conversation_connect_request:
                                 
                                 detail::reply( state, std::move( request), lookup, []( auto& state, auto& correlation, auto code)
                                 {
                                    common::message::conversation::connect::Reply reply;
                                    reply.correlation = correlation;
                                    reply.code.result = code;
                                    tcp::send( state, reply);
                                 });
                                 break;

                              default:
                                 log::line( log::category::error, code::casual::internal_unexpected_value, " message type: ", common::message::type( request), " - action: discard");

                           }

                        };  
                     }

                  } // lookup

                  namespace call
                  {
                     auto reply = basic_forward< common::message::service::call::Reply>;
                  } // call

               } // service

               namespace conversation
               {
                  namespace connect
                  {
                     auto reply( State& state)
                     {
                        return [&state]( common::message::conversation::connect::Reply& message)
                        {
                           Trace trace{ "gateway::group::inbound::handle::local::internal::conversation::connect::reply"};
                           common::log::line( verbose::log, "message: ", message);

                           // we consume the correlation to connection, and add a conversation specific "route"
                           if( auto descriptor = tcp::send( state, message))
                           {
                              state.conversations.push_back( state::Conversation{ message.correlation, descriptor, message.process});
                              common::log::line( verbose::log, "state.conversations: ", state.conversations);
                           }
                        };
                     }
                     
                  } // connect

                  auto send( State& state)
                  {
                     return [&state]( common::message::conversation::callee::Send& message)
                     {
                        Trace trace{ "gateway::group::inbound::handle::local::internal::conversation::send"};
                        common::log::line( verbose::log, "message: ", message);

                        if( auto found = algorithm::find( state.conversations, message.correlation))
                        {
                           local::tcp::send( state, found->descriptor, message);
                           
                           // if the `send` has event that indicate that it will end the conversation - we remove our "route" state
                           // for the connection
                           if( message.code.result != decltype( message.code.result)::absent)
                              state.conversations.erase( std::begin( found));
                        }
                        else
                           common::log::line( common::log::category::error, "(internal) failed to correlate conversation: ", message.correlation);

                     };
                  }
                  
               } // conversation

               namespace queue
               {
                  namespace lookup
                  {
                     auto reply( State& state)
                     {
                        return [&state]( casual::queue::ipc::message::lookup::Reply& message)
                        {
                           Trace trace{ "gateway::group::inbound::handle::local::internal::queue::lookup::reply"};
                           common::log::line( verbose::log, "message: ", message);

                           auto request = state.pending.requests.consume( message.correlation);

                           if( message.process)
                           {
                              state.multiplex.send( message.process.ipc, std::move( request));
                              return;
                           }

                           // queue not available - send error reply
               
                           auto send_error = [&state, &message]( auto&& reply)
                           {
                              reply.correlation = message.correlation;
                              tcp::send( state, reply);
                           };

                           switch( request.type())
                           {
                              using Enum = decltype( request.type());
                              case Enum::queue_group_dequeue_request:
                              {
                                 send_error( casual::queue::ipc::message::group::dequeue::Reply{});
                                 break;
                              }
                              case Enum::queue_group_enqueue_request:
                              {
                                 send_error( casual::queue::ipc::message::group::enqueue::Reply{});
                                 break;
                              }
                              default:
                                 common::log::line( common::log::category::error, "unexpected message type for queue request: ", message, " - action: drop message");
                           }
                        };
                     }
                  } // lookup

                  namespace dequeue
                  {
                     auto reply = basic_forward< casual::queue::ipc::message::group::dequeue::Reply>;
                  } // dequeue

                  namespace enqueue
                  {
                     auto reply = basic_forward< casual::queue::ipc::message::group::enqueue::Reply>;
                  } // enqueue
               } // queue

               namespace domain
               {
                  auto connected( State& state)
                  {
                     return [&state]( const gateway::message::domain::Connected& message)
                     {
                        Trace trace{ "gateway::group::inbound::handle::local::internal::domain::connected"};
                        common::log::line( verbose::log, "message: ", message);

                        state.external.connected( state.directive, message);

                        common::log::line( verbose::log, "state.external: ", state.external);
       
                     };
                  }

                  namespace discovery
                  {
                     auto reply = basic_forward< casual::domain::message::discovery::Reply>;

                     namespace topology::implicit
                     {
                        auto update( State& state)
                        {
                           return [&state]( const casual::domain::message::discovery::topology::implicit::Update& message)
                           {
                              Trace trace{ "gateway::group::inbound::handle::local::internal::domain::discovery::topology::implicit::update"};
                              common::log::line( verbose::log, "message: ", message);

                              auto send_if_compatible = [ &state, &message]( auto descriptor)
                              {
                                 auto connection = state.external.connection( descriptor);
                                 CASUAL_ASSERT( connection);

                                 if( ! message::protocol::compatible( message, connection->protocol()))
                                    return;

                                 auto information = casual::assertion( state.external.information( descriptor), 
                                    "failed to find information for descriptor: ", descriptor);

                                 if( information->configuration.discovery != decltype( information->configuration.discovery)::forward)
                                    return;

                                 // check if domain has seen this message before...
                                 if( algorithm::find( message.domains, information->domain))
                                    return;

                                 local::tcp::send( state, descriptor, message);
                              };

                              algorithm::for_each( state.external.descriptors(), send_if_compatible);

                           };
                        }

                     } // topology::implicit

                  } // discovery
               } // domain


               namespace transaction
               {
                  namespace resource
                  {
                     template< typename Message>
                     auto basic_clean( State& state)
                     {
                        return [&state]( Message& message)
                        {
                           Trace trace{ "gateway::group::inbound::handle::local::internal::transaction::resource::detail::basic_clean"};
                           common::log::line( verbose::log, "message: ", message);

                           if( auto descriptor = state.consume( message.correlation))
                           {
                              
                              if constexpr( std::is_same_v< Message, common::message::transaction::resource::prepare::Reply>)
                              {
                                 // prepare can be optimize with read-only, or detect some error. We know that if 
                                 // it is not "ok", we will not get commit/rollback and need to clean state. 
                                 if( message.state != decltype( message.state)::ok)
                                    state.in_flight_cache.remove( descriptor, message.trid);
                              }
                              else
                                 state.in_flight_cache.remove( descriptor, message.trid);

                              if( tcp::send( state, descriptor, message))
                                 return;

                              log::line( log::category::error, code::casual::communication_unavailable, " transaction - failed to send ", message.type(), " to descriptor: ", descriptor);
                              return;
                           }

                           log::line( log::category::error, code::casual::communication_unavailable, " transaction - failed to correlate ", message.type());
                        };
                     }
                     
                     namespace prepare
                     {
                        auto reply = resource::basic_clean< common::message::transaction::resource::prepare::Reply>;
                     } // prepare
                     namespace commit
                     {
                        //! when we get this reply, we know the transaction is 'done', at least in this domain (and downstream)
                        auto reply = resource::basic_clean< common::message::transaction::resource::commit::Reply>;
                     } // commit
                     namespace rollback
                     {
                        //! when we get this reply, we know the transaction is 'done', at least in this domain (and downstream)
                        auto reply = resource::basic_clean< common::message::transaction::resource::rollback::Reply>;
                     } // commit
                  } // resource
               } // transaction

            } // internal

            namespace external
            {
               namespace detail
               {
                  template< typename T>
                  using has_trid = decltype( std::declval< T&>().trid);   
               } // detail

               template< typename M>
               auto correlate( State& state, const M& message)
               {
                  if constexpr( common::traits::detect::is_detected_v< detail::has_trid, M>)
                  {
                     state.in_flight_cache.add( state.external.last(), message.trid);
                  }

                  state.correlations.emplace_back( message.correlation, state.external.last());
                  return state.external.last();
               }

               namespace disconnect
               {
                  auto reply( State& state)
                  {
                     return [&state]( const gateway::message::domain::disconnect::Reply& message)
                     {
                        Trace trace{ "gateway::group::inbound::handle::local::external::disconnect::reply"};
                        common::log::line( verbose::log, "message: ", message);

                        if( auto descriptor = state.consume( message.correlation))
                        {
                           common::log::line( verbose::log, "descriptor: ", descriptor);

                           if( algorithm::find( state.correlations, descriptor) || ! state.in_flight_cache.empty( descriptor))
                           {
                              common::log::line( verbose::log, "state.correlations: ", state.correlations, ", state.in_flight_cache: ", state.in_flight_cache);

                              // connection still got pending stuff to do, we keep track until its 'idle'.
                              state.pending.disconnects.push_back( descriptor);
                           }
                           else 
                           {
                              // connection is 'idle', we just 'loose' the connection
                              handle::connection::lost( state, descriptor);

                              // we return false to tell the dispatch not to try consume any more messages on
                              // this connection
                              return false;
                           }
                        }
                        return true;
                     };
                  }
                  
               } // disconnect

               namespace service
               {
                  namespace detail
                  {
                     template< typename M>
                     auto lookup( State& state, strong::file::descriptor::id descriptor, M&& message, common::message::service::lookup::request::context::Semantic semantic)
                        -> std::enable_if_t< std::is_rvalue_reference_v< decltype( message)>>
                     {
                        Trace trace{ "gateway::group::inbound::handle::local::external::service::detail::lookup"};
                        common::log::line( verbose::log, "message: ", message);

                        // Change 'sender' so we get the reply
                        message.process = common::process::handle();

                        // Prepare lookup
                        common::message::service::lookup::Request request{ common::process::handle()};
                        {
                           // get the information (with an assertion)
                           auto information = casual::assertion( state.external.information( descriptor), "invalid descriptor: ", descriptor, ", message: ", message);
                           request.correlation = message.correlation;
                           request.requested = message.service.name;
                           request.context.semantic = semantic;

                           request.context.requester = information->configuration.discovery == decltype( information->configuration.discovery)::forward ?
                              decltype( request.context.requester)::external_discovery : decltype( request.context.requester)::external;
                        }

                        // Add message to pending (we know message is an rvalue, so it's going to be a move)
                        state.pending.requests.add( std::forward< M>( message));

                        // Send lookup
                        state.multiplex.send( ipc::manager::service(), request); 

                     }
                  } // detail
                  namespace call
                  {
                     auto request( State& state)
                     {
                        return [&state]( common::message::service::call::callee::Request& message)
                        {
                           Trace trace{ "gateway::group::inbound::handle::local::external::service::call::request"};
                           log::line( verbose::log, "message: ", message);
                           
                           auto descriptor = external::correlate( state, message);

                           using namespace common::message::service::lookup::request;
                           auto semantics =  message.flags.exist( decltype( message.flags.type())::no_reply) ? context::Semantic::forward : context::Semantic::no_busy_intermediate;
                           detail::lookup( state, descriptor, std::move( message), semantics);
                        };
                     }
                  } // call
               } // service

               namespace conversation
               {
                  namespace connect
                  {
                     auto request( State& state)
                     {
                        return [&state]( common::message::conversation::connect::callee::Request& message)
                        {
                           Trace trace{ "gateway::group::inbound::handle::local::external::conversation::connect::request"};
                           log::line( verbose::log, "message: ", message);
                           
                           auto descriptor = external::correlate( state, message);
                           service::detail::lookup( state, descriptor, std::move( message), common::message::service::lookup::request::context::Semantic::no_busy_intermediate);
                        };
                     }
                     
                  } // connect

                  auto disconnect( State& state)
                  {
                     return [&state]( common::message::conversation::Disconnect& message)
                     {
                        Trace trace{ "gateway::group::inbound::handle::local::external::conversation::disconnect"};
                        common::log::line( verbose::log, "message: ", message);

                        if( auto found = algorithm::find( state.conversations, message.correlation))
                        {
                           state.multiplex.send( found->process.ipc, message);

                           // we're done with this connection, regardless of what callee thinks...
                           state.conversations.erase( std::begin( found));
                        }
                     };
                  }

                  auto send( State& state)
                  {
                     return [&state]( common::message::conversation::callee::Send& message)
                     {
                        Trace trace{ "gateway::group::inbound::handle::local::external::conversation::send"};
                        common::log::line( verbose::log, "message: ", message);

                        if( auto found = algorithm::find( state.conversations, message.correlation))
                           state.multiplex.send( found->process.ipc, message);
                        else
                           common::log::line( common::log::category::error, "(external) failed to correlate conversation: ", message.correlation);
                     };
                  }
                  
               } // conversation


               namespace queue
               {
                  namespace lookup
                  {
                     template< typename M>
                     bool send( State& state, strong::file::descriptor::id descriptor, M&& message)
                     {
                        Trace trace{ "gateway::group::inbound::handle::local::external::queue::lookup::send"};

                        // Prepare queue lookup
                        casual::queue::ipc::message::lookup::Request request{ common::process::handle()};
                        {
                           request.correlation = message.correlation;
                           request.name = message.name;

                           auto information = casual::assertion( state.external.information( descriptor), "invalid descriptor: ", descriptor, ", message: ", message);

                           using Enum = decltype( request.context.requester);
                           request.context.requester = information->configuration.discovery == decltype( information->configuration.discovery)::forward ?
                              Enum::external_discovery : Enum::external;
                           
                        }

                        if( state.multiplex.send( ipc::manager::optional::queue(), request))
                        {
                           // Change 'sender' so we get the reply
                           message.process = common::process::handle();

                           // Add message to buffer
                           state.pending.requests.add( std::forward< M>( message));

                           return true;
                        }

                        return false;

                     }
                  } // lookup

                  namespace enqueue
                  {
                     auto request( State& state)
                     {
                        return [&state]( casual::queue::ipc::message::group::enqueue::Request& message)
                        {
                           Trace trace{ "gateway::group::inbound::handle::local::external::queue::enqueue::Request"};
                           common::log::line( verbose::log, "message: ", message);

                           external::correlate( state, message);

                           // Send lookup
                           if( ! queue::lookup::send( state, state.external.last(), message))
                           {
                              common::log::line( common::log::category::error, "failed to lookup queue: ", message.name, " - action: send error reply");

                              casual::queue::ipc::message::group::enqueue::Reply reply;
                              reply.correlation = message.correlation;
                              reply.execution = message.execution;

                              // empty uuid represent error. TODO: is this enough? No, it is not.
                              reply.id = common::uuid::empty();

                              tcp::send( state, reply);
                           }
                        };
                     }
                  } // enqueue

                  namespace dequeue
                  {
                     auto request( State& state)
                     {
                        return [&state]( casual::queue::ipc::message::group::dequeue::Request& message)
                        {
                           Trace trace{ "gateway::group::inbound::handle::queue::dequeue::Request::operator()"};
                           common::log::line( verbose::log, "message: ", message);

                           external::correlate( state, message);

                           // Send lookup
                           if( ! queue::lookup::send( state, state.external.last(), message))
                           {
                              common::log::line( common::log::category::error, "failed to lookup queue: ", message.name, " - action: send error reply");

                              casual::queue::ipc::message::group::enqueue::Reply reply;
                              reply.correlation = message.correlation;
                              reply.execution = message.execution;

                              // empty uuid represent error. TODO: is this enough? No, it is not.
                              reply.id = common::uuid::empty();

                              tcp::send( state, reply);
                           }
                        };
                     }
                  } // dequeue
               } // queue

               namespace domain
               {
                  namespace discovery
                  {
                     auto request( State& state)
                     {
                        return [&state]( casual::domain::message::discovery::Request& message)
                        {
                           Trace trace{ "gateway::inbound::handle::local::external::domain::discovery::request"};
                           common::log::line( verbose::log, "message: ", message);

                           auto descriptor = external::correlate( state, message);

                           // Set 'sender' so we get the reply
                           message.process = common::process::handle();

                           const auto information = state.external.information( descriptor);
                           assert( information);

                           if( information->configuration.discovery == decltype( information->configuration.discovery)::forward)
                              message.directive = decltype( message.directive)::forward;

                           casual::domain::discovery::request( message);                    
                        };
                     }
                  } // discovery
               } // domain
         
               namespace transaction
               {
                  namespace resource
                  {
                     template< typename Message>
                     auto basic_request( State& state)
                     {
                        return [&state]( Message& message)
                        {
                           Trace trace{ "gateway::inbound::handle::local::external::transaction::basic_request"};
                           common::log::line( verbose::log, "message: ", message);

                           external::correlate( state, message);

                           // Set 'sender' so we get the reply
                           message.process = common::process::handle();
                           state.multiplex.send(ipc::manager::transaction(), message);
                        };
                     }

                     namespace prepare
                     {
                        auto request = basic_request< common::message::transaction::resource::prepare::Request>;
                     } // prepare
                     namespace commit
                     {
                        auto request = basic_request< common::message::transaction::resource::commit::Request>;
                     } // commit
                     namespace rollback
                     {
                        auto request = basic_request< common::message::transaction::resource::rollback::Request>;
                     } // commit
                  } // resource
               } // transaction
            } // external

         } // <unnamed>
      } // local

      internal_handler internal( State& state)
      {
         casual::domain::discovery::provider::registration( casual::domain::discovery::provider::Ability::topology);

         return internal_handler{

            // lookup
            local::internal::service::lookup::reply( state),
            
            // service
            local::internal::service::call::reply( state),

            // conversation
            local::internal::conversation::connect::reply( state),
            local::internal::conversation::send( state),

            // queue
            local::internal::queue::lookup::reply( state),
            local::internal::queue::dequeue::reply( state),
            local::internal::queue::enqueue::reply( state),

            // domain discovery
            local::internal::domain::discovery::reply( state),
            local::internal::domain::discovery::topology::implicit::update( state),

            // transaction
            local::internal::transaction::resource::prepare::reply( state),
            local::internal::transaction::resource::commit::reply( state),
            local::internal::transaction::resource::rollback::reply( state),

            local::internal::domain::connected( state),
         };
      }

      external_handler external( State& state)
      {
         return external_handler{
            
            local::external::disconnect::reply( state),

            // service call
            local::external::service::call::request( state),

            // conversation
            local::external::conversation::connect::request( state),
            local::external::conversation::disconnect( state),
            local::external::conversation::send( state),

            // queue
            local::external::queue::enqueue::request( state),
            local::external::queue::dequeue::request( state),

            // discover
            local::external::domain::discovery::request( state),

            // transaction
            local::external::transaction::resource::prepare::request( state),
            local::external::transaction::resource::commit::request( state),
            local::external::transaction::resource::rollback::request( state)
         };
      }


      namespace connection
      {
         message::inbound::connection::Lost lost( State& state, common::strong::file::descriptor::id descriptor)
         {
            Trace trace{ "gateway::group::inbound::handle::connection::lost"};
            log::line( verbose::log, "descriptor: ", descriptor);

            auto extracted = state.extract( descriptor);

            if( ! extracted.empty())
            {
               log::line( log::category::error, code::casual::communication_unavailable, " lost connection - address: ", extracted.information.configuration.address, ", domain: ", extracted.information.domain);
               log::line( log::category::verbose::error, "extracted: ", extracted);

               // discard service lookup
               // There is no other pending we need to discard at the moment.
               algorithm::for_each( extracted.pending.services, [ &state]( auto& call)
               {
                  common::message::service::lookup::discard::Request request{ process::handle()};
                  request.correlation = call.correlation;
                  request.requested = call.service.name;
                  // we don't need the reply
                  request.reply = false;

                  state.multiplex.send( ipc::manager::service(), request);
               });            
            }

            return { std::move( extracted.information.configuration), std::move( extracted.information.domain)};
         }

         void disconnect( State& state, common::strong::file::descriptor::id descriptor)
         {
            Trace trace{ "gateway::group::inbound::handle::connection::disconnect"};
            log::line( verbose::log, "descriptor: ", descriptor);

            if( auto connection = state.external.connection( descriptor))
            {
               log::line( verbose::log, "connection: ", *connection);

               if( message::protocol::compatible< message::domain::disconnect::Request>( connection->protocol()))
               {
                  if( auto correlation = local::tcp::send( state, descriptor, message::domain::disconnect::Request{}))
                     state.correlations.emplace_back( correlation, descriptor);
               }
               else
               {
                  handle::connection::lost( state, descriptor);
               }
            }
         }
         
      } // connection


      void idle( State& state)
      {
         Trace trace{ "gateway::group::inbound::handle::idle"};

         // take care of pending disconnects
         if( ! state.pending.disconnects.empty())
         {
            auto connection_done = [&state]( auto descriptor)
            {
               return state.disconnectable( descriptor);
            };

            auto done = algorithm::container::extract( state.pending.disconnects, algorithm::filter( state.pending.disconnects, connection_done));

            for( auto descriptor : done)
               handle::connection::lost( state, descriptor);
         }
      }

      void shutdown( State& state)
      {
         Trace trace{ "gateway::group::inbound::handle::shutdown"};

         state.runlevel = decltype( state.runlevel())::shutdown;

         // try to do a 'soft' disconnect. copy - connection::disconnect mutates external
         for( auto descriptor : state.external.descriptors())
            handle::connection::disconnect( state, descriptor);
      }

      void abort( State& state)
      {
         Trace trace{ "gateway::group::inbound::handle::abort"};

         state.runlevel = decltype( state.runlevel())::error;

         // 'kill' all sockets, and try to take care of pending stuff. copy - connection::lost mutates external
         for( auto descriptor : state.external.descriptors())
            handle::connection::lost( state, descriptor);
      }
      

   } // gateway::group::inbound::handle

} // casual