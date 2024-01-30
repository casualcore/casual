//!
//! Copyright (c) 2023, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/group/inbound/task/create.h"
#include "gateway/group/inbound/tcp.h"

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
               auto context( State& state, common::strong::socket::id descriptor)
               {
                  using result_context = std::decay_t< C>;

                  auto information = casual::assertion( state.external.information( descriptor), "invalid descriptor: ", descriptor);

                  if( information->configuration.discovery == decltype( information->configuration.discovery)::forward)
                     return result_context::external_discovery;
                  else 
                     return result_context::external;
               }


               template< typename M>
               void branch( State& state, const M& message)
               {
                  auto gtrid = transaction::id::range::global( message.trid);

                  common::message::transaction::coordinate::inbound::Request request{ common::process::handle()};
                  request.correlation = message.correlation;
                  request.execution = message.execution;
                  request.gtrid = gtrid;

                  state.multiplex.send( ipc::manager::transaction(), request);
               }

               template< typename M>
               void service( State& state, common::strong::socket::id descriptor, const M& message)
               {
                  common::message::service::lookup::Request request{ common::process::handle()};
                  request.correlation = message.correlation;
                  request.execution = message.execution;
                  request.requested = message.service.name;
                  request.context.requester = lookup::context< decltype( request.context.requester)>( state, descriptor);

                  using Semantic = common::message::service::lookup::request::context::Semantic;
                  
                  // for service calls, it might be a _no-reply_ request.
                  if constexpr( std::same_as< M, common::message::service::call::callee::Request>)
                     request.context.semantic = message.flags.exist( common::message::service::call::request::Flag::no_reply) ? 
                        Semantic::no_reply : Semantic::no_busy_intermediate;
                  else
                     request.context.semantic = Semantic::no_busy_intermediate;

                  if( message.trid)
                     request.gtrid = transaction::id::range::global( message.trid);

                  // Send lookup
                  state.multiplex.send( ipc::manager::service(), request);
               }

               template< typename M>
               void queue( State& state, common::strong::socket::id descriptor, const M& message)
               {
                  casual::queue::ipc::message::lookup::Request request{ common::process::handle()};
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


            template< typename M>
            void push_service_error_reply( strong::correlation::id correlation, strong::execution::id execution , common::code::xatmi code)
            {
               M reply;
               reply.correlation = correlation;
               reply.execution = execution;
               reply.code.result = code;
               ipc::inbound().push( std::move( reply));
            }

            template< typename M>
            void push_queue_error_reply( strong::correlation::id correlation, strong::execution::id execution)
            {
               M reply;
               reply.correlation = correlation;
               reply.execution = execution;
               ipc::inbound().push( std::move( reply));
            }

            template< typename M>
            void push_transaction_error_reply( strong::correlation::id correlation, strong::execution::id execution, strong::resource::id resource, common::code::xa code)
            {
               M reply;
               reply.correlation = correlation;
               reply.execution = execution;
               reply.state = code;
               reply.resource = resource;
               ipc::inbound().push( std::move( reply));
            }


            namespace send
            {
               template< typename M> 
               void service_request( State& state, const common::message::service::lookup::Reply& lookup, M&& message)
               {
                  using reply_type = common::message::reverse::type_t< M>;

                  // take care of "route mapping"
                  if( message.service.name != lookup.service.name)
                     message.service.requested = std::exchange( message.service.name, lookup.service.name);

                  switch( lookup.state)
                  {
                     using Enum = decltype( lookup.state);
                     case Enum::idle:
                     {
                        state.multiplex.send( lookup.process.ipc, message, []( auto& destination, auto& complete)
                        {
                           log::error( code::xatmi::service_error, "destination: ", destination, " is unreachable - action: reply with: ", code::xatmi::service_error);

                           // add a reply to our inbound to emulate a real reply
                           local::push_service_error_reply< reply_type>( complete.correlation(), complete.execution(), code::xatmi::service_error);
                        });
                        break;
                     }
                     case Enum::absent:
                     {
                        log::error( code::xatmi::no_entry, "service: ", lookup.service, " is not handled by this domain (any more) - action: reply with: ", code::xatmi::no_entry);
                        // add a reply to our inbound to emulate a real reply
                        local::push_service_error_reply< reply_type>( message.correlation, message.execution, code::xatmi::no_entry);
                        break;
                     }
                     default: 
                     {
                        log::error( code::casual::internal_unexpected_value, "unexpected state on lookup reply: ", lookup, " - action: reply with: ", code::xatmi::service_error);
                        // add a reply to our inbound to emulate a real reply
                        local::push_service_error_reply< reply_type>( message.correlation, message.execution, code::xatmi::system);
                        break;
                     }
                  }
               }

               template< typename M> 
               void queue_request( State& state, const casual::queue::ipc::message::lookup::Reply& lookup, M& message)
               {
                  using reply_type = common::message::reverse::type_t< M>;

                  if( ! lookup.process.ipc)
                     return local::push_queue_error_reply< reply_type>( message.correlation, message.execution);

                  state.multiplex.send( lookup.process.ipc, message, []( auto& destination, auto& complete)
                  {
                     log::error( code::xatmi::service_error, "destination: ", destination, " is unreachable - action: reply with empty reply");

                     // add a reply to our inbound to emulate a real reply
                     local::push_queue_error_reply< reply_type>( complete.correlation(), complete.execution());
                  });
               }


            } // send



            template< typename M>
            auto handle_service( State& state, common::strong::socket::id descriptor, M& message)
            {
               Trace trace{ "gateway::group::inbound::task::create::local::handle_service"};

               using reply_type = common::message::reverse::type_t< M>;

               struct Shared
               {
                  common::strong::socket::id descriptor;
                  common::transaction::ID origin_trid;
                  M message;
                  std::optional< common::message::service::lookup::Reply> lookup;
               };

               local::lookup::service( state, descriptor, message);

               auto shared = std::make_shared< Shared>();

               if( message.trid)
               {
                  shared->origin_trid = message.trid;

                  if( auto found = state.transaction_cache.associate( descriptor, message.trid))
                     message.trid = *found;
                  else
                     local::lookup::branch( state, message);
               }
               
               shared->message = std::move( message);
               shared->descriptor = descriptor;
               // make sure we get the reply
               shared->message.process = process::handle();

               return casual::task::concurrent::Unit{
                  [ &state, shared]( common::message::service::lookup::Reply& reply) mutable
                  {
                     Trace trace{ "gateway::group::inbound::task::create::service::call lookup::Reply"};

                     if( reply.correlation != shared->message.correlation)
                        return casual::task::concurrent::unit::Dispatch::pending;

                     shared->lookup = std::move( reply);

                     // if the call wasn't in transaction, or we've branched the trid already
                     if( ! shared->origin_trid || shared->origin_trid != shared->message.trid)
                        local::send::service_request( state, *shared->lookup, shared->message);
                     
                     return casual::task::concurrent::unit::Dispatch::pending;

                  },
                  [ &state, shared]( common::message::transaction::coordinate::inbound::Reply& reply) mutable
                  {
                     Trace trace{ "gateway::group::inbound::task::create::service::call transaction::coordinate::inbound::Reply"};

                     if( reply.correlation != shared->message.correlation)
                        return casual::task::concurrent::unit::Dispatch::pending;

                     state.transaction_cache.add_branch( shared->descriptor, reply.trid);

                     shared->message.trid = std::move( reply.trid);

                     if( shared->lookup)
                        local::send::service_request( state, *shared->lookup, shared->message);

                  
                     return casual::task::concurrent::unit::Dispatch::pending;
                  },
                  [ &state, shared]( reply_type& reply)
                  {
                     Trace trace{ "gateway::group::inbound::task::create::local reply_type"};

                     if( reply.correlation != shared->message.correlation)
                        return casual::task::concurrent::unit::Dispatch::pending;

                     if constexpr( concepts::same_as< reply_type, common::message::service::call::Reply>)
                     {
                        reply.transaction.trid = std::move( shared->origin_trid);
                        inbound::tcp::send( state, reply);
                     }

                     if constexpr( concepts::same_as< reply_type, common::message::conversation::connect::Reply>)
                     {
                        // we consume the correlation to connection, and add a conversation specific "route"
                        if( auto descriptor = inbound::tcp::send( state, reply))
                        {
                           state.conversations.push_back( state::Conversation{ reply.correlation, descriptor, reply.process});
                           common::log::line( verbose::log, "state.conversations: ", state.conversations);
                        }
                     }
                     
                     // regardless we're done.
                     return casual::task::concurrent::unit::Dispatch::done;
                  },
                  [ &state, shared]( const casual::task::concurrent::message::Conclude& message)
                  {
                     if( message.correlation != shared->message.correlation)
                        return casual::task::concurrent::unit::Dispatch::pending;

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
            auto handle_queue( State& state, common::strong::socket::id descriptor, M& message)
            {
               Trace trace{ "gateway::group::inbound::task::create::local::handle_queue"};

               using reply_type = common::message::reverse::type_t< M>;

               local::lookup::queue( state, descriptor, message);

               struct Shared
               {
                  strong::socket::id descriptor;
                  M message;
                  bool wait_for_branch = false;
                  std::optional< casual::queue::ipc::message::lookup::Reply> lookup; 
               };

               auto shared = std::make_shared< Shared>();

               if( message.trid)
               {
                  if( auto found = state.transaction_cache.associate( descriptor, message.trid))
                     message.trid = *found;
                  else
                  {
                     shared->wait_for_branch = true;
                     local::lookup::branch( state, message);
                  }
               }

               shared->message = std::move( message);
               shared->descriptor = descriptor;
               shared->message.process = process::handle();

               return casual::task::concurrent::Unit{
                  [ &state, shared]( casual::queue::ipc::message::lookup::Reply& reply) mutable
                  {
                     Trace trace{ "gateway::group::inbound::task::create::local::handle_queue lookup::Reply"};

                     if( reply.correlation != shared->message.correlation)
                        return casual::task::concurrent::unit::Dispatch::pending;

                     shared->lookup = std::move( reply);

                     // We wait for the trid if we've not got it yet.
                     if( ! shared->wait_for_branch)
                        local::send::queue_request( state, *shared->lookup, shared->message);

                     return casual::task::concurrent::unit::Dispatch::pending;
                  },
                  [ &state, shared]( common::message::transaction::coordinate::inbound::Reply& reply) mutable
                  {
                     Trace trace{ "gateway::group::inbound::task::create::local::handle_queue transaction::lookup::Reply"};
                     
                     if( reply.correlation != shared->message.correlation)
                        return casual::task::concurrent::unit::Dispatch::pending;

                     state.transaction_cache.add_branch( shared->descriptor, reply.trid);

                     shared->message.trid = std::move( reply.trid);
                     shared->wait_for_branch = false;

                     if( shared->lookup)
                        local::send::queue_request( state, *shared->lookup, shared->message);

                     return casual::task::concurrent::unit::Dispatch::pending;
                  },
                  [ &state, shared]( reply_type& reply)
                  {
                     Trace trace{ "gateway::group::inbound::task::create::local::handle_queue reply_type"};

                     if( reply.correlation != shared->message.correlation)
                        return casual::task::concurrent::unit::Dispatch::pending;
                     
                     inbound::tcp::send( state, reply);

                     // regardless we're done.
                     return casual::task::concurrent::unit::Dispatch::done;
                  },
               };
            }

            template< typename M>
            auto handle_transaction( State& state, common::strong::socket::id descriptor, M& message)
            {
               Trace trace{ "gateway::group::inbound::task::create::local::handle_transaction"};

               using reply_type = common::message::reverse::type_t< M>;

               struct Shared
               {
                  common::strong::socket::id descriptor;
                  strong::correlation::id correlation;
                  common::transaction::ID origin_trid;
               };

               auto shared = Shared{ descriptor, message.correlation, message.trid};

               // map to the internal trid, we expect to find this in cache
               if( auto found = state.transaction_cache.find( common::transaction::id::range::global( message.trid)))
               {
                  message.trid = *found;
                  message.process = process::handle();
                  state.multiplex.send( ipc::manager::transaction(), message);
               }
               else
               {
                  log::error( code::casual::invalid_semantics, "failed map to internal trid: ", message, " - action: reply with: ", code::xa::protocol);
                  local::push_transaction_error_reply< reply_type>( message.correlation, message.execution, message.resource, code::xa::protocol);
               }

               return casual::task::concurrent::Unit{
                  [ &state, shared = std::move( shared)]( reply_type& reply)
                  {
                     if( reply.correlation != shared.correlation)
                        return casual::task::concurrent::unit::Dispatch::pending;

                     // map back to the external trid.
                     reply.trid = shared.origin_trid;

                     // clean up cache, with the external trid
                     if constexpr( concepts::same_as< reply_type, common::message::transaction::resource::prepare::Reply>)
                     {
                        if( reply.state == code::xa::read_only)
                           state.transaction_cache.dissociate( shared.descriptor, reply.trid);
                     }
                     else
                        state.transaction_cache.dissociate( shared.descriptor, reply.trid);

                     inbound::tcp::send( state, reply);
                     
                     // we're done regardless
                     return casual::task::concurrent::unit::Dispatch::done;
                  }
               };
            }
         } // <unnamed>
      } // local

      namespace service
      {
         casual::task::concurrent::Unit call( State& state, common::strong::socket::id descriptor, common::message::service::call::callee::Request&& message)
         {
            Trace trace{ "gateway::group::inbound::task::create::service::call"};
            return local::handle_service( state, descriptor, message);
         }

         casual::task::concurrent::Unit conversation( State& state, common::strong::socket::id descriptor, common::message::conversation::connect::callee::Request&& message)
         {
            Trace trace{ "gateway::group::inbound::task::create::service::conversation"};
            return local::handle_service( state, descriptor, message);
         }
         
      } // service

      namespace queue
      {
         
         casual::task::concurrent::Unit enqueue( State& state, common::strong::socket::id descriptor, casual::queue::ipc::message::group::enqueue::Request&& message)
         {
            Trace trace{ "gateway::group::inbound::task::create::queue::enqueue"};
            return local::handle_queue( state, descriptor, message);

         }

         casual::task::concurrent::Unit dequeue( State& state, common::strong::socket::id descriptor, casual::queue::ipc::message::group::dequeue::Request&& message)
         {
            Trace trace{ "gateway::group::inbound::task::create::queue::dequeue"};
            return local::handle_queue( state, descriptor, message);
         }

      } // queue

   
      casual::task::concurrent::Unit transaction( State& state, common::strong::socket::id descriptor, common::message::transaction::resource::prepare::Request&& message)
      {
         return local::handle_transaction( state, descriptor, message);
      }

      casual::task::concurrent::Unit transaction( State& state, common::strong::socket::id descriptor, common::message::transaction::resource::commit::Request&& message)
      {
         return local::handle_transaction( state, descriptor, message);
      }

      casual::task::concurrent::Unit transaction( State& state, common::strong::socket::id descriptor, common::message::transaction::resource::rollback::Request&& message)
      {
         return local::handle_transaction( state, descriptor, message);
      }

   } // gateway::group::inbound::task::create
   
} // casual
