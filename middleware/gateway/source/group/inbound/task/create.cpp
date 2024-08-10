//!
//! Copyright (c) 2023, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/group/inbound/task/create.h"
#include "gateway/group/inbound/tcp.h"
#include "gateway/message/protocol/transform.h"
#include "gateway/message/protocol.h"

#include "gateway/common.h"
#include "gateway/group/ipc.h"

namespace casual
{
   using namespace common;

   namespace gateway::group::inbound::task::create
   {
      namespace local
      {
         namespace
         {
            namespace lookup
            {
               template< typename C>
               auto context( State& state, strong::socket::id descriptor)
               {
                  using result_context = std::decay_t< C>;

                  auto information = casual::assertion( state.connections.information( descriptor), "invalid descriptor: ", descriptor);

                  if( information->configuration.discovery == decltype( information->configuration.discovery)::forward)
                     return result_context::external_discovery;
                  else 
                     return result_context::external;
               }


               template< typename M>
               void branch( State& state, strong::socket::id descriptor, const M& message)
               {
                  auto gtrid = transaction::id::range::global( message.trid);

                  common::message::transaction::inbound::branch::Request request{ state.connections.process_handle( descriptor)};
                  request.correlation = message.correlation;
                  request.execution = message.execution;
                  request.gtrid = gtrid;

                  state.multiplex.send( ipc::manager::transaction(), request);
               }

               template< typename M>
               void service( State& state, strong::socket::id descriptor, const M& message)
               {
                  common::message::service::lookup::Request request{ state.connections.process_handle( descriptor)};
                  request.correlation = message.correlation;
                  request.execution = message.execution;
                  request.requested = message.service.name;
                  request.context.requester = lookup::context< decltype( request.context.requester)>( state, descriptor);

                  using Semantic = common::message::service::lookup::request::context::Semantic;
                  
                  // for service calls, it might be a _no-reply_ request.
                  if constexpr( std::same_as< M, common::message::service::call::callee::Request>)
                     request.context.semantic = flag::contains( message.flags, common::message::service::call::request::Flag::no_reply) ? 
                        Semantic::no_reply : Semantic::regular;
                  else
                     request.context.semantic = Semantic::regular;

                  if( message.trid)
                     request.gtrid = transaction::id::range::global( message.trid);

                  // Send lookup
                  state.multiplex.send( ipc::manager::service(), request);
               }

               template< typename M>
               void queue( State& state, strong::socket::id descriptor, const M& message)
               {
                  casual::queue::ipc::message::lookup::Request request{ state.connections.process_handle( descriptor)};
                  {
                     request.correlation = message.correlation;
                     request.execution = message.execution;
                     request.name = message.name;
                     request.context.requester = lookup::context< decltype( request.context.requester)>( state, descriptor);
                     request.context.semantic = decltype( request.context.semantic)::direct;
                  }

                  if( message.trid)
                     request.gtrid = common::transaction::id::range::global( message.trid);

                  //! if queue-manager is offline for some reason, we emulate "no-ent";
                  if( ! state.multiplex.send( ipc::manager::optional::queue(), request))
                     ipc::inbound().push( common::message::reverse::type( request));
               }

            } // lookup

            // tries to send to the "partner ipc", if it's full/busy we try later via state.multiplex.send abstraction
            template< typename M>
            void send_to_partner_ipc( State& state, strong::socket::id descriptor, M&& message)
            {
               if( auto found = state.connections.find_internal( descriptor))
                  state.multiplex.send( found->connector().handle().ipc(), std::forward< M>( message));
               else
                  log::error( code::casual::internal_correlation, "failed to correlate the ipc partner for tcp descriptor:  ", descriptor);
            }
         

            template< typename M>
            void fake_service_error_reply( State& state, strong::socket::id descriptor, M&& request, common::code::xatmi code)
            {
               auto reply = common::message::reverse::type( request);
               reply.code.result = code;
               local::send_to_partner_ipc( state, descriptor, std::move( reply));
            }

            template< typename M>
            void fake_conversation_connect_error_reply( State& state, strong::socket::id descriptor, M&& request, common::code::xatmi code)
            {
               auto reply = common::message::reverse::type( request);
               reply.code.result = code;
               local::send_to_partner_ipc( state, descriptor, std::move( reply));
            }  

            template< typename M>
            void fake_queue_error_reply( State& state, strong::socket::id descriptor, M&& request)
            {
               local::send_to_partner_ipc( state, descriptor, common::message::reverse::type( request));
            }

            template< typename M>
            void fake_transaction_error_reply( State& state, strong::socket::id descriptor, const M& message, common::code::xa code)
            {
               auto reply = common::message::reverse::type( message);
               reply.state = code;
               reply.trid = message.trid;
               reply.resource = message.resource;

               local::send_to_partner_ipc( state, descriptor, std::move( reply));
            }


            namespace send
            {
               template< typename M> 
               void service_request( State& state, strong::socket::id descriptor, const common::message::service::lookup::Reply& lookup, M&& message)
               {
                  // take care of "route mapping"
                  if( message.service.name != lookup.service.name)
                     message.service.requested = std::exchange( message.service.name, lookup.service.name);

                  switch( lookup.state)
                  {
                     using message_type = std::decay_t< M>;

                     using Enum = decltype( lookup.state);
                     case Enum::idle:
                     {
                        state.multiplex.send( lookup.process.ipc, message, [ &state, descriptor]( auto& destination, auto& complete)
                        {
                           log::error( code::xatmi::service_error, "destination: ", destination, " is unreachable - action: reply with: ", code::xatmi::service_error);

                           // deserialize the request add a reply to our inbound to emulate a real reply
                           local::fake_service_error_reply( state, descriptor, serialize::native::complete< message_type>( complete), code::xatmi::service_error);
                        });
                        break;
                     }
                     case Enum::absent:
                     {
                        log::error( code::xatmi::no_entry, "service: ", lookup.service.name, " is not handled by this domain (any more) - action: reply with: ", code::xatmi::no_entry);
                        // add a reply to our inbound to emulate a real reply
                        local::fake_service_error_reply( state, descriptor, message, code::xatmi::no_entry);
                        break;
                     }
                     default: 
                     {
                        log::error( code::casual::internal_unexpected_value, "unexpected state on lookup reply: ", lookup, " - action: reply with: ", code::xatmi::service_error);
                        // add a reply to our inbound to emulate a real reply
                        local::fake_service_error_reply( state, descriptor, message, code::xatmi::system);
                        break;
                     }
                  }
               }

               template< typename M> 
               void queue_request( State& state, strong::socket::id descriptor, const casual::queue::ipc::message::lookup::Reply& lookup, M& message)
               {
                  if( ! lookup.process.ipc)
                     return local::fake_queue_error_reply( state, descriptor, message);

                  state.multiplex.send( lookup.process.ipc, message, [ &state, descriptor]( auto& destination, auto& complete)
                  {
                     log::error( code::xatmi::service_error, "destination: ", destination, " is unreachable - action: reply with empty reply");

                     // Deserialize the request and add the reply inbound to emulate a real reply
                     local::fake_queue_error_reply( state, descriptor, serialize::native::complete< M>( complete));
                  });
               }


            } // send


            auto handle_service( State& state, strong::socket::id descriptor, common::message::service::call::callee::Request&& message)
            {
               Trace trace{ "gateway::group::inbound::task::create::local::handle_service"};

               struct Shared
               {
                  common::transaction::ID origin_trid;
                  common::message::service::call::callee::Request message;
                  std::optional< common::message::service::lookup::Reply> lookup;
               };

               local::lookup::service( state, descriptor, message);

               auto shared = std::make_shared< Shared>();

               if( message.trid)
               {
                  shared->origin_trid = message.trid;

                  if( auto found = state.transaction_cache.associate( message.trid, descriptor))
                     message.trid = *found;
                  else
                     local::lookup::branch( state, descriptor, message);
               }
               
               shared->message = std::move( message);
               // make sure the ipc partner to the tcp socket gets the reply
               shared->message.process = state.connections.process_handle( descriptor);

               return task_unit{ descriptor, message.correlation,
                  [ &state, shared]( common::message::service::lookup::Reply& reply, strong::socket::id descriptor) mutable
                  {
                     Trace trace{ "gateway::group::inbound::task::create::service::call lookup::Reply"};

                     switch( reply.state)
                     {
                        using Enum = decltype( reply.state);
                        case Enum::idle:
                        {
                           shared->lookup = std::move( reply);

                           // If the call wasn't in transaction, or we've branched the trid already.
                           // If the call failed, we've "sent" the reply, 
                           if( ! shared->origin_trid || shared->origin_trid != shared->message.trid)
                              local::send::service_request( state, descriptor, *shared->lookup, shared->message);

                           break;
                        }
                        case Enum::absent:
                           local::fake_service_error_reply( state, descriptor, shared->message, code::xatmi::no_entry);
                           break;
                        case Enum::timeout:
                           local::fake_service_error_reply( state, descriptor, shared->message, code::xatmi::timeout);
                           break;
                     }
                     
                     return casual::task::concurrent::unit::Dispatch::pending;

                  },
                  [ &state, shared]( common::message::transaction::inbound::branch::Reply& reply, strong::socket::id descriptor) mutable
                  {
                     Trace trace{ "gateway::group::inbound::task::create::service::call transaction::inbound::branch::Reply"};

                     state.transaction_cache.add( reply.trid, descriptor);

                     shared->message.trid = std::move( reply.trid);

                     if( shared->lookup)
                        local::send::service_request( state, descriptor, *shared->lookup, shared->message);

                     return casual::task::concurrent::unit::Dispatch::pending;
                  },
                  [ &state, shared]( common::message::service::call::Reply& reply, strong::socket::id descriptor)
                  {
                     Trace trace{ "gateway::group::inbound::task::create::local reply_type"};

                     auto connection = state.connections.find_external( descriptor);
                     CASUAL_ASSERT( connection);

                     if( message::protocol::compatible< common::message::service::call::Reply>( connection->protocol()))
                        tcp::send( state, connection->descriptor(), reply);
                     else
                        tcp::send( state, connection->descriptor(), message::protocol::transform::to< common::message::service::call::v1_2::Reply>( std::move( reply)));
                     
                     // We're done
                     return casual::task::concurrent::unit::Dispatch::done;
                  },
                  [ &state, shared]( const casual::task::concurrent::message::task::Failed& message, strong::socket::id descriptor)
                  {
                     if( ! shared->lookup)
                     {
                        // we have a pending lookup, discard.
                        common::message::service::lookup::discard::Request request;
                        request.correlation = message.correlation;
                        request.execution = message.execution;
                        request.reply = false;
                        request.requested = shared->message.service.name;

                        state.multiplex.send( ipc::manager::service(), request);
                     }

                     return casual::task::concurrent::unit::Dispatch::done;
                  }
               };
            }


            auto handle_conversation( State& state, strong::socket::id descriptor, common::message::conversation::connect::callee::Request&& message)
            {
               Trace trace{ "gateway::group::inbound::task::create::local::handle_conversation"};

               // connect::Reply and Send from internal we just forward to the corresponding tcp socket, 
               // no need to handle them in this task (we don't need any information from those messages)

               struct Shared
               {
                  common::transaction::ID origin_trid;
                  common::message::conversation::connect::callee::Request message;
                  std::optional< common::message::service::lookup::Reply> lookup;
               };

               local::lookup::service( state, descriptor, message);

               auto shared = std::make_shared< Shared>();

               if( message.trid)
               {
                  shared->origin_trid = message.trid;

                  if( auto found = state.transaction_cache.associate( message.trid, descriptor))
                     message.trid = *found;
                  else
                     local::lookup::branch( state, descriptor, message);
               }
               
               shared->message = std::move( message);
               // make sure the ipc partner to the tcp socket gets the reply
               shared->message.process = state.connections.process_handle( descriptor);

               return task_unit{ descriptor, message.correlation,
                  [ &state, shared]( common::message::service::lookup::Reply& reply, strong::socket::id descriptor) mutable
                  {
                     Trace trace{ "gateway::group::inbound::task::create::local::handle_conversation lookup::Reply"};

                     switch( reply.state)
                     {
                        using Enum = decltype( reply.state);
                        case Enum::idle:
                        {
                           shared->lookup = std::move( reply);

                           // if the call wasn't in transaction, or we've branched the trid already
                           if( ! shared->origin_trid || shared->origin_trid != shared->message.trid)
                              local::send::service_request( state, descriptor, *shared->lookup, shared->message);

                           break;
                        }
                        case Enum::absent:
                           local::fake_conversation_connect_error_reply( state, descriptor, shared->message, code::xatmi::no_entry);
                           break;
                        case Enum::timeout:
                           local::fake_conversation_connect_error_reply( state, descriptor, shared->message, code::xatmi::timeout);
                           break;
                     }
                     
                     return casual::task::concurrent::unit::Dispatch::pending;

                  },
                  [ &state, shared]( common::message::transaction::inbound::branch::Reply& reply, strong::socket::id descriptor) mutable
                  {
                     Trace trace{ "gateway::group::inbound::task::create::local::handle_conversation transaction::inbound::branch::Reply"};

                     state.transaction_cache.add( reply.trid, descriptor);

                     shared->message.trid = std::move( reply.trid);

                     if( shared->lookup)
                        local::send::service_request( state, descriptor, *shared->lookup, shared->message);

                  
                     return casual::task::concurrent::unit::Dispatch::pending;
                  },
                  [ &state, shared]( common::message::conversation::callee::Send& message, strong::socket::id descriptor)
                  {
                     Trace trace{ "gateway::group::inbound::task::create::local::handle_conversation Send"};

                     CASUAL_ASSERT( shared->lookup);

                     // we send the message to the looked up destination
                     state.multiplex.send( shared->lookup->process.ipc, message);
                     
                     // we're not done with the conversation
                     return casual::task::concurrent::unit::Dispatch::pending;
                  },
                  [ &state, shared]( common::message::conversation::Disconnect& message, strong::socket::id descriptor)
                  {
                     Trace trace{ "gateway::group::inbound::task::create::local::handle_conversation Disconnect"};

                     CASUAL_ASSERT( shared->lookup);

                     // we send the message to the looked up destination
                     state.multiplex.send( shared->lookup->process.ipc, message);
                     
                     // Now we're done.
                     return casual::task::concurrent::unit::Dispatch::done;
                  },
                  [ &state, shared]( const casual::task::concurrent::message::task::Failed& message, strong::socket::id descriptor)
                  {
                     if( ! shared->lookup)
                     {
                        // we have a pending lookup, discard.
                        common::message::service::lookup::discard::Request request;
                        request.correlation = message.correlation;
                        request.execution = message.execution;
                        request.reply = false;
                        request.requested = shared->message.service.name;

                        state.multiplex.send( ipc::manager::service(), request);
                     }

                     return casual::task::concurrent::unit::Dispatch::done;
                  }
               };
            }

            template< typename M>
            auto handle_queue( State& state, strong::socket::id descriptor, M& message)
            {
               Trace trace{ "gateway::group::inbound::task::create::local::handle_queue"};

               // The enqueue/dequeue Reply we just forward to the corresponding tcp socket

               local::lookup::queue( state, descriptor, message);

               struct Shared
               {
                  M message;
                  bool wait_for_branch = false;
                  std::optional< casual::queue::ipc::message::lookup::Reply> lookup; 
               };

               auto shared = std::make_shared< Shared>();

               if( message.trid)
               {
                  if( auto found = state.transaction_cache.associate( message.trid, descriptor))
                  {
                     message.trid = *found;
                  }
                  else
                  {
                     shared->wait_for_branch = true;
                     local::lookup::branch( state, descriptor,  message);
                  }
               }

               shared->message = std::move( message);
               shared->message.process = state.connections.process_handle( descriptor);

               return task_unit{ descriptor, message.correlation,
                  [ &state, shared]( casual::queue::ipc::message::lookup::Reply& reply, strong::socket::id descriptor) mutable
                  {
                     Trace trace{ "gateway::group::inbound::task::create::local::handle_queue lookup::Reply"};

                     shared->lookup = std::move( reply);

                     // We wait for the trid if we've not got it yet.
                     if( shared->wait_for_branch)
                        return casual::task::concurrent::unit::Dispatch::pending;

                     local::send::queue_request( state, descriptor, *shared->lookup, shared->message);
                     return casual::task::concurrent::unit::Dispatch::done;
                  },
                  [ &state, shared]( common::message::transaction::inbound::branch::Reply& reply, strong::socket::id descriptor) mutable
                  {
                     Trace trace{ "gateway::group::inbound::task::create::local::handle_queue transaction::lookup::Reply"};

                     state.transaction_cache.add( reply.trid, descriptor);

                     shared->message.trid = std::move( reply.trid);
                     shared->wait_for_branch = false;

                     if( ! shared->lookup)
                        return casual::task::concurrent::unit::Dispatch::pending;
                     
                     local::send::queue_request( state, descriptor, *shared->lookup, shared->message);
                     return casual::task::concurrent::unit::Dispatch::done;
                  }
               };
            }

            template< typename M>
            auto handle_transaction( State& state, strong::socket::id descriptor, M&& message)
            {
               Trace trace{ "gateway::group::inbound::task::create::local::handle_transaction"};

               using reply_type = common::message::reverse::type_t< M>;

               // map to the internal trid, we expect to find this in cache
               if( auto found = state.transaction_cache.find( common::transaction::id::range::global( message.trid)))
               {
                  message.trid = *found;
                  message.process = state.connections.process_handle( descriptor);
                  state.multiplex.send( ipc::manager::transaction(), message);
               }
               else
               {
                  log::error( code::casual::invalid_semantics, "failed map to internal trid: ", message, " - action: reply with: ", code::xa::protocol);
                  local::fake_transaction_error_reply( state, descriptor, message, code::xa::protocol);
               }

               return task_unit{ descriptor, message.correlation,
                  [ &state, origin_trid = message.trid]( reply_type& reply, strong::socket::id descriptor)
                  {
                     Trace trace{ "gateway::group::inbound::task::create::local::handle_transaction task"};

                     // map back to the external trid.
                     reply.trid = origin_trid;

                     // we don't clean transaction_cache, TM will tell us when it's done

                     inbound::tcp::send( state, descriptor, reply);
                     
                     // we're done regardless
                     return casual::task::concurrent::unit::Dispatch::done;
                  }
               };
            }
         } // <unnamed>
      } // local

      namespace service
      {
         task_unit call( State& state, strong::socket::id descriptor, common::message::service::call::callee::Request&& message)
         {
            Trace trace{ "gateway::group::inbound::task::create::service::call"};
            return local::handle_service( state, descriptor, std::move( message));
         }

         task_unit conversation( State& state, strong::socket::id descriptor, common::message::conversation::connect::callee::Request&& message)
         {
            Trace trace{ "gateway::group::inbound::task::create::service::conversation"};
            return local::handle_conversation( state, descriptor, std::move( message));
         }
         
      } // service

      namespace queue
      {
         
         task_unit enqueue( State& state, strong::socket::id descriptor, casual::queue::ipc::message::group::enqueue::Request&& message)
         {
            Trace trace{ "gateway::group::inbound::task::create::queue::enqueue"};
            return local::handle_queue( state, descriptor, message);

         }

         task_unit dequeue( State& state, strong::socket::id descriptor, casual::queue::ipc::message::group::dequeue::Request&& message)
         {
            Trace trace{ "gateway::group::inbound::task::create::queue::dequeue"};
            return local::handle_queue( state, descriptor, message);
         }

      } // queue

   
      task_unit transaction( State& state, strong::socket::id descriptor, common::message::transaction::resource::prepare::Request&& message)
      {
         return local::handle_transaction( state, descriptor, message);
      }

      task_unit transaction( State& state, strong::socket::id descriptor, common::message::transaction::resource::commit::Request&& message)
      {
         return local::handle_transaction( state, descriptor, message);
      }

      task_unit transaction( State& state, strong::socket::id descriptor, common::message::transaction::resource::rollback::Request&& message)
      {
         return local::handle_transaction( state, descriptor, message);
      }

   } // gateway::group::inbound::task::create
   
} // casual
