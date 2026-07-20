//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "queue/forward/handle.h"

#include "queue/forward/state.h"
#include "queue/common/log.h"
#include "queue/common/ipc.h"
#include "queue/common/ipc/message.h"

#include "common/message/transaction.h"
#include "common/message/dispatch.h"
#include "common/message/dispatch/handle.h"
#include "common/message/internal.h"
#include "common/message/service.h"
#include "common/event/send.h"
#include "common/event/listen.h"
#include "common/communication/ipc.h"
#include "common/communication/instance.h"
#include "common/exception/guard.h"

namespace casual
{
   using namespace common;

   namespace queue::forward
   {
      namespace local
      {
         namespace
         {
            namespace pending
            {
               template< typename P>
               auto consume( P& pending, const strong::correlation::id& correlation) -> std::optional< std::ranges::range_value_t< P>>
               {
                  if( auto found = common::algorithm::find( pending, correlation))
                     return algorithm::container::extract( pending, std::begin( found));

                  common::log::debug( "failed to extract correlation: ", correlation);
                  return std::nullopt;
               }

            } // pending

            namespace transform::forward
            {
               auto service( State& state)
               {
                  return []( auto& service)
                  {
                     state::forward::Service result;
                     result.alias = service.alias;
                     result.source.queue = service.source;
                     
                     if( service.reply)
                     {
                        state::forward::Service::Reply reply;
                        reply.queue = service.reply.value().queue;
                        reply.delay = service.reply.value().delay;
                        result.reply = std::move( reply);
                     }
                     
                     result.target.service = service.target.service;
                     result.instances.configured = service.instances;
                     result.instances.enabled = service.enabled;

                     return result;
                  };
               }
               auto queue( State& state)
               {
                  return []( auto& queue)
                  {
                     state::forward::Queue result;
                     result.alias = queue.alias;
                     result.source.queue = queue.source;

                     result.target.queue = queue.target.queue;
                     result.target.delay = queue.target.delay;
                     
                     result.instances.configured = queue.instances;
                     result.instances.enabled = queue.enabled;
                     return result;
                  };
               } 
            } // transform::forward

            namespace queue::lookup
            {
               void source( State& state, auto& forward)
               {
                  ipc::message::lookup::Request request{ process::handle()};
                  request.name = forward.source.queue;
                  request.context.semantic = decltype( request.context.semantic)::wait;

                  state::pending::queue::source::Lookup pending;
                  pending.id = forward.id;
                  // send request and keep the correlation for our state.
                  pending.correlation = state.multiplex.send( ipc::queue::manager(), request);

                  common::log::debug( "request: ", request);

                  state.pending.queue.lookup.source.push_back( std::move( pending));
               }

               void target( State& state, auto&& pending, const state::forward::queue::Target& target, common::buffer::Payload&& payload)
               {
                  // lookup target queue
                  {
                     ipc::message::lookup::Request request{ process::handle()};
                     request.correlation = pending.correlation;
                     request.name = target.queue;
                     // we'll wait 'forever'
                     request.context.semantic = decltype( request.context.semantic)::wait;

                     state.multiplex.send( ipc::queue::manager(), request);
                  }

                  forward::state::pending::queue::target::Lookup lookup{ std::move( pending)};
                  lookup.buffer = std::move( payload);
                  lookup.delay = target.delay;

                  common::log::debug( "lookup: ", lookup);

                  state.pending.queue.lookup.target.push_back( std::move( lookup));
               }
               
            } // queue::lookup


            namespace send
            {
               namespace dequeue
               {
                  template< typename F>
                  bool request( State& state, F& forward, const state::pending::queue::source::Lookup& pending, const ipc::message::lookup::Reply& lookup)
                  {
                     Trace trace{ "queue::forward::send::dequeue::request"};
                     common::log::debug( "forward: ", forward);
                     
                     casual::assertion( pending.correlation == lookup.correlation, "correlation mismatch - pending: ", pending, " lookup: ", lookup);

                     ipc::message::group::dequeue::Request request{ common::process::handle()};
                     request.trid = common::transaction::id::create( common::process::id());
                     request.correlation = lookup.correlation;
                     request.queue = lookup.queue;
                     request.name = lookup.name;
                     request.selector = forward.selector;
                     request.block = true;

                     if( state.multiplex.send( lookup.process.ipc, request))
                     {
                        state::pending::Dequeue dequeue{ { pending}};
                        dequeue.trid = request.trid;
                        dequeue.ipc = lookup.process.ipc;

                        state.pending.dequeues.push_back( std::move( dequeue));

                        return true;
                     }

                     return false;
                  }
               } // dequeue

               namespace enqueue
               {
                  bool request( State& state, state::pending::queue::target::Lookup& pending, const ipc::message::lookup::Reply& lookup)
                  {
                     Trace trace{ "queue::forward::send::enqueue::request"};
                     common::log::debug( "pending: ", pending);

                     ipc::message::group::enqueue::Request request{ common::process::handle()};
                     request.correlation = lookup.correlation;
                     request.name = lookup.name;
                     request.queue = lookup.queue;
                     request.trid = pending.trid;
                     request.message.payload = std::move( pending.buffer);

                     if( pending.delay > common::chronology::duration::zero())
                        request.message.attributes.available = platform::time::clock::type::now() + pending.delay;

                     if( state.multiplex.send( lookup.process.ipc, request))
                     {
                        state::pending::Enqueue enqueue{ pending};
                        enqueue.id = pending.id;
                        enqueue.correlation = lookup.correlation;

                        state.pending.enqueues.push_back( std::move( enqueue));
                        return true;
                     }
                     return false;
                  }
          
               } // enqueue

               namespace transaction
               {
                  namespace rollback
                  {
                     template< typename P>
                     void request( State& state, const P& pending)
                     {
                        Trace trace{ "queue::forward::send::transaction::rollback::request"};
                        common::log::debug( "pending: ", pending);

                        common::message::transaction::rollback::Request request{ common::process::handle()};
                        request.correlation = pending.correlation;
                        request.trid = pending.trid;

                        state.multiplex.send( ipc::transaction::manager(), request);

                        state.pending.transaction.rollbacks.emplace_back( pending);
                     }
                  } // rollback

                  namespace commit
                  {
                     template<  typename P>
                     void request( State& state, P&& pending)
                     {
                        Trace trace{ "queue::forward::send::transaction::commit::request"};
                        common::log::debug( "pending: ", pending);

                        common::message::transaction::commit::Request request{ common::process::handle()};
                        request.correlation = pending.correlation;
                        request.trid = pending.trid;

                        state.multiplex.send( ipc::transaction::manager(), request);

                        state.pending.transaction.commits.emplace_back( std::forward< P>( pending));
                     }
                  } // rollback

               } // transaction
            } // send

            namespace detail::send::discard
            {
               template< typename F>
               void request( State& state, const F& forward, const state::pending::Dequeue& pending)
               {
                  ipc::message::group::dequeue::forget::Request request{ process::handle()};
                  request.correlation = pending.correlation;
                  
                  state.multiplex.send( pending.ipc, request);
               }

               template< typename F>
               void request( State& state, const F& forward, const state::pending::service::Lookup& pending)
               {
                  message::service::lookup::discard::Request request{ process::handle()};
                  request.correlation = pending.correlation;

                  state.multiplex.send( ipc::service::manager(), request);
               }

               template< typename F>
               void request( State& state, const F& forward, const state::pending::queue::source::Lookup& pending)
               {
                  ipc::message::lookup::discard::Request request{ process::handle()};
                  request.correlation = pending.correlation;

                  state.multiplex.send( ipc::queue::manager(), request);
               }

               template< typename F>
               void request( State& state, const F& forward, const state::pending::queue::target::Lookup& pending)
               {
                  ipc::message::lookup::discard::Request request{ process::handle()};
                  request.correlation = pending.correlation;

                  state.multiplex.send( ipc::queue::manager(), request);
               }

            } // detail::send::discard

            namespace detail::machine
            {
               // potentially start flows for forward. 
               template< typename F>
               void next( State& state, F& forward)
               {
                  // send lookup for all "missing instances"
                  common::algorithm::for_n( forward.instances.missing(), [&state, &forward]()
                  {
                     queue::lookup::source( state, forward);
                     // increment running instances, since we have sent a lookup for a missing instance
                     ++forward.instances;
                  });
               }

               void end_instance_flow( State& state, const auto& pending)
               {
                  state.forward_apply( pending.id, [&]( auto& forward)
                  {
                     --forward.instances;
                     detail::machine::next( state, forward);
                  });
               }

            } // detail::machine

            namespace comply::to
            {
               void state( State& state)
               {
                  Trace trace{ "queue::forward::service::local::comply::to::state"};

                  // Forwards will wait forever for dequeues and service lookups, so we cancel enough
                  // to ensure that enough flows will reach a final state
                  {
                     auto cancel_surplus_instances = [ &state]( const auto& forward)
                     {
                        auto surplus = forward.instances.surplus();
                        if( surplus == 0)
                           return;

                        auto get_pendings = [ &forward]( auto& pendings)
                        {
                           return algorithm::filter( pendings, [ &forward]( const auto& pending){ return pending.id == forward.id;});
                        };

                        auto send_discard_request = [ &state, &forward, &surplus]( const auto& pending)
                        {
                           // guard to be able to use this predicate in filter algorithm
                           if( surplus == 0)
                              return false;

                           detail::send::discard::request( state, forward, pending);
                           --surplus;
                           
                           return true;
                        };

                        // take care of pending source lookups
                        {
                           auto pending = algorithm::filter( get_pendings( state.pending.queue.lookup.source), send_discard_request);
                           auto discarded = algorithm::container::extract( state.pending.queue.lookup.source, pending);
                           common::algorithm::container::append( discarded, state.pending.discard.lookup.queue.source);
                        }

                        // take care of pending dequeues
                        {
                           auto pending = algorithm::filter( get_pendings( state.pending.dequeues), send_discard_request);
                           auto discarded = algorithm::container::extract( state.pending.dequeues, pending);
                           common::algorithm::container::append( discarded, state.pending.discard.dequeues);
                        }

                        if constexpr( std::is_same_v< std::remove_cvref_t< decltype( forward)>, state::forward::Service>)
                        {
                           // extract and discard all (0..1) pending lookups for the forward, and put'em in lookup-lookup discard 
                           auto pending = algorithm::filter( get_pendings( state.pending.service.lookups), send_discard_request);
                           auto discarded = algorithm::container::extract( state.pending.service.lookups, pending);
                           for( auto& pending : discarded)
                              state.pending.discard.lookup.service.emplace_back( std::move( pending));
                        }

                        // take care of pending target lookups
                        {
                           auto pending = algorithm::filter( get_pendings( state.pending.queue.lookup.target), send_discard_request);
                           auto discarded = algorithm::container::extract( state.pending.queue.lookup.target, pending);
                           for( auto& pending : discarded)
                              state.pending.discard.lookup.queue.target.emplace_back( std::move( pending));
                        }
                     };

                     algorithm::for_each( state.forward.queues, cancel_surplus_instances);
                     algorithm::for_each( state.forward.services, cancel_surplus_instances);
                  } 

                  // start the "flows" for forwards, that has missing instances.
                  auto start_flow = []( auto& state, auto& forwards)
                  {
                     for( auto& forward : forwards)
                        detail::machine::next( state, forward);

                  };

                  // start lookups on queues, even if we got them already.
                  start_flow( state, state.forward.services);
                  start_flow( state, state.forward.queues);

                  log::debug( "state: ", state);

               }
            } // comply::to

            namespace handle
            {
               namespace configuration::update
               {
                  auto request( State& state)
                  {
                     return [&state]( const ipc::message::forward::group::configuration::update::Request& message)
                     { 
                        Trace trace{ "queue::forward::service::local::handle::configuration::update::request"};
                        log::debug( "message: ", message);

                        state.alias = message.model.alias;
                        state.memberships = message.model.memberships;

                        // TODO maintenance we only update instances
                        auto add_or_update = []( auto& source, auto& target, auto transform)
                        {
                           auto handle_source = [&]( auto& forward)
                           {
                              auto is_alias = [ &alias = forward.alias]( auto& forward){ return forward.alias == alias;};

                              if( auto found = algorithm::find_if( target, is_alias))
                              {
                                 found->instances.configured = forward.instances;
                                 found->instances.enabled = forward.enabled;
                              }
                              else
                              {
                                 target.push_back( transform( forward));
                              }
                           };

                           algorithm::for_each( source, handle_source);
                        };

                        add_or_update( message.model.services, state.forward.services, transform::forward::service( state));
                        add_or_update( message.model.queues, state.forward.queues, transform::forward::queue( state));

                        comply::to::state( state);

                        state.runlevel = decltype( state.runlevel())::running;

                        state.multiplex.send( message.process.ipc, common::message::reverse::type( message, process::handle()));
                     };
                  }

               } // configuration::update

               namespace queue::lookup
               {
                  auto reply( State& state)
                  {
                     return [&state]( const ipc::message::lookup::Reply& message)
                     {
                        Trace trace{ "queue::forward::service::local::handle::queue::lookup::reply"};
                        log::debug( "message: ", message);

                        // the lookup reply can be for a source queue or a target queue. Source queue 
                        // is the first in the "flow". And target queue, could be a queue -> queue forward,
                        // or a queue -> service -> enqueue (service-reply).

                        if( auto pending = pending::consume( state.pending.queue.lookup.source, message.correlation))
                        {
                           log::debug( "source: ", *pending);
                           
                           // if we've got a source queue, we can send a dequeue request.
                           // Otherwise we need to end the flow for this instance, and possibly start a new flow (if there are missing instances).
                           if( message)
                           {
                              state.forward_apply( pending->id, [&]( auto& forward)
                              {
                                 if( ! send::dequeue::request( state, forward, *pending, message))
                                    detail::machine::end_instance_flow( state, *pending);
                              });
                           }
                           else
                           {
                              detail::machine::end_instance_flow( state, *pending);
                           }
                        }
                        else if( auto pending = pending::consume( state.pending.queue.lookup.target, message.correlation))
                        {
                           log::debug( "target: ", *pending);

                           state.forward_apply( pending->id, [&]( auto& forward)
                           {
                              // if we've got a target queue, we can send an enqueue request.
                              // Otherwise we need to rollback the transaction.
                              if( message && send::enqueue::request( state, *pending, message))
                                 ; // no-op
                              else
                                 send::transaction::rollback::request( state, *pending);
                           });

                        }
                        else
                           log::debug( "failed to find pending queue lookup: ", message);

                     };
                  }
                  namespace discard
                  {
                     auto reply( State& state)
                     {
                        return [&state]( const ipc::message::lookup::discard::Reply& message)
                        {
                           Trace trace{ "queue::forward::handle::queue::lookup::discard::reply"};
                           common::log::debug( "message: ", message);

                           if( auto pending = pending::consume( state.pending.discard.lookup.queue.source, message.correlation))
                           {
                              log::debug( "discarded source lookup: ", *pending);

                              // this instance's flow has been interrupted, we need to decrement the instance and try to start a new flow.
                              detail::machine::end_instance_flow( state, *pending);                              
                           }
                           else if( auto pending = pending::consume( state.pending.discard.lookup.queue.target, message.correlation))
                           {
                              log::debug( "discarded target lookup: ", *pending);

                              // we have an ongoing transaction, that we need to rollback.
                              send::transaction::rollback::request( state, std::move( *pending));
                           }
                           else
                              log::debug( "failed to find pending queue lookup discard: ", message);
                        };
                     }

                  } // discard

               } // queue::lookup

               namespace dequeue
               {
                  auto reply( State& state)
                  {
                     return [&state]( ipc::message::group::dequeue::Reply& message)
                     {
                        Trace trace{ "queue::forward::service::local::handle::dequeue::reply"};
                        log::debug( "message: ", message);

                        auto pending = pending::consume( state.pending.dequeues, message.correlation);

                        if( ! pending)
                           return;
                        
                        if( ! message.message)
                        {
                           log::line( log::category::error, "empty dequeued message - action: rollback");
                           send::transaction::rollback::request( state, std::move( *pending));
                        }
                        else if( state.runlevel > decltype( state.runlevel())::running)
                        {
                           // if we're in _shutdown mode_ we don't want to start any "flows"
                           log::line( log::category::information, "message dequeued in shutdown mode - the message might end up on error queue - action: rollback");
                           send::transaction::rollback::request( state, std::move( *pending));
                        }
                        else if( auto forward = state.forward_service( pending->id))
                        {
                           // lookup service
                           {
                              message::service::lookup::Request request{ process::handle()};
                              request.correlation = pending->correlation;
                              request.requested = forward->target.service;
                              // we'll wait 'forever'
                              request.context.semantic = decltype( request.context.semantic)::wait;
                              // make sure we get a unique execution id for this 'context', will be present in the actual call later on.
                              request.execution = decltype( request.execution)::generate();

                              state.multiplex.send( ipc::service::manager(), request);
                           }

                           forward::state::pending::service::Lookup lookup{ std::move( *pending)};
                           lookup.buffer = std::move( message.message->payload);

                           state.pending.service.lookups.push_back( std::move( lookup));

                           log::debug( "state.pending.service.lookups: ", state.pending.service.lookups);
                        }
                        else if( auto forward = state.forward_queue( pending->id))
                        {
                           local::queue::lookup::target( state, std::move( *pending), forward->target, std::move( message.message->payload));
                        }
                     };
                  }

                  namespace forget
                  {
                     namespace detail::discard::pending
                     {
                        template< typename M>
                        auto dequeue( State& state, const M& message)
                        {
                           Trace trace{ "queue::forward::handle::dequeue::forget::detail::discard::pending::dequeue"};

                           if( auto pending = local::pending::consume( state.pending.dequeues, message.correlation))
                           {
                              common::log::debug( "pending: ", *pending);

                              state.forward_apply( pending->id, []( auto& forward)
                              {
                                 --forward.instances;
                                 common::log::debug( "forward: ", forward);
                              });

                              common::log::debug( "state.pending.dequeues: ", state.pending.dequeues);
                           }
                        }
                        
                     } // detail::discard::pending

                     // Possible final state of the casual-forward "state-machine"
                     auto request( State& state)
                     {
                        // we get this from queue group if it's 'going down' or some other configuration
                        // changes...
                        return [&state]( const ipc::message::group::dequeue::forget::Request& message)
                        {                                 
                           Trace trace{ "queue::forward::handle::dequeue::forget::request"};
                           common::log::debug( "message: ", message);

                           detail::discard::pending::dequeue( state, message);
                        };
                     }

                     // Possible final state of the casual-forward "state-machine"
                     auto reply( State& state)
                     {
                        // We've requested to forget the blocking dequeue. 
                        return [&state]( const ipc::message::group::dequeue::forget::Reply& message)
                        {                                 
                           Trace trace{ "queue::forward::handle::dequeue::forget::reply"};
                           common::log::debug( "message: ", message);

                           auto pending = local::pending::consume( state.pending.discard.dequeues, message.correlation);

                           if( ! pending)
                              return;

                           if( message.discarded)
                           {
                              state.forward_apply( pending->id, [ &state]( auto& forward)
                              {
                                 --forward.instances;
                                 common::log::debug( "forward: ", forward);

                                 // We try to restart the flow
                                 local::detail::machine::next( state, forward);
                              });
                           }
                           else
                           {
                              send::transaction::rollback::request( state, std::move( *pending));
                           }
                        };
                     }
                  } // forget

               } // dequeue

               namespace service
               {
                  namespace lookup
                  {
                     auto reply( State& state)
                     {
                        return [&state]( message::service::lookup::Reply& message)
                        {
                           Trace trace{ "queue::forward::service::local::handle::service::lookup::reply"};
                           log::debug( "message: ", message);

                           auto pending = pending::consume( state.pending.service.lookups, message.correlation);
                           
                           if( ! pending)
                           {
                              // if we cant find a pending lookup we assume the lookup is discarded and handle it later
                              if( ! algorithm::find( state.pending.discard.lookup.service, message.correlation))
                                 log::error( common::code::casual::invalid_semantics, "expected pending service-lookup-discard for correlation: ", message.correlation);

                              return;
                           }

                           if( message.state != decltype( message.state)::idle)
                           {
                              log::error( common::code::xatmi::no_entry, "service not callable: ", message.service.name);
                              send::transaction::rollback::request( state, std::move( *pending));
                              return;
                           }

                           message::service::call::caller::Request request{ buffer::payload::Send{ pending->buffer}, process::handle()};
                           request.update( message);

                           request.trid = pending->trid;

                           state.multiplex.send( message.process.ipc, request);
                           state.pending.service.calls.emplace_back( std::move( *pending), message.process);
                        };
                     }

                     namespace discard
                     {
                        auto reply( State& state)
                        {
                           return [&state]( const message::service::lookup::discard::Reply& message)
                           {
                              Trace trace{ "queue::forward::service::local::handle::service::lookup::discard::reply"};
                              log::debug( "message: ", message);

                              auto pending = pending::consume( state.pending.discard.lookup.service, message.correlation);
                              log::debug( "pending lookup_discard: ", pending);

                              if( ! pending)
                                 return;

                              // rollback the 'flow'
                              send::transaction::rollback::request( state, std::move( *pending));
                           };
                        }

                     } // discard
                  } // lookup
                  
                  namespace call
                  {
                     auto reply( State& state)
                     {
                        return [&state]( message::service::call::Reply& message)
                        {
                           Trace trace{ "queue::forward::service::local::handle::service::call::reply"};
                           log::debug( "message: ", message);

                           auto pending = pending::consume( state.pending.service.calls, message.correlation);

                           if( ! pending)
                              return;

                           // we know that this is a service forward
                           auto forward = state.forward_service( pending->id);
                           assert( forward);

                           if( message.code.result != common::code::xatmi::ok)
                           {
                              send::transaction::rollback::request( state, std::move( *pending));
                              return;
                           }

                           if( forward->reply)
                              local::queue::lookup::target( state, std::move( *pending), *forward->reply, std::move( message.buffer));
                           else
                              send::transaction::commit::request( state, std::move( *pending));

                        };
                     }
                  } // call
               } // service

               namespace enqueue
               {
                  auto reply( State& state)
                  {
                     return [&state]( const ipc::message::group::enqueue::Reply& message)
                     {
                        Trace trace{ "queue::forward::handle::enqueue::reply"};
                        common::log::debug( "message: ", message);

                        auto pending = pending::consume( state.pending.enqueues, message.correlation);

                        if( ! pending)
                           return;

                        if( message.id)
                           send::transaction::commit::request( state, std::move( *pending));
                        else
                           send::transaction::rollback::request( state, std::move( *pending));
                     };
                  }
               }

               namespace transaction
               {
                  namespace rollback
                  {
                     // Possible final state of the casual-forward "state-machine"
                     auto reply( State& state)
                     {
                        return [&state]( const common::message::transaction::rollback::Reply& message)
                        {                                    
                           Trace trace{ "queue::forward::handle::transaction::rollback::reply"};
                           common::log::debug( "message: ", message);

                           auto pending = pending::consume( state.pending.transaction.rollbacks, message.correlation);

                           if( ! pending)
                              return;

                           if( message.stage != decltype( message.stage)::rollback)
                              common::log::error( message.state, "failed to rollback - trid: ", message.trid);

                           state.forward_apply( pending->id, [&state]( auto& forward)
                           {
                              --forward.instances;
                              ++forward.metric.rollback.count;
                              forward.metric.rollback.last = platform::time::clock::type::now();

                              detail::machine::next( state, forward);
                           });
                        };
                     }

                  } // rollback

                  namespace commit
                  {
                     // Possible final state of the casual-forward "state-machine"
                     auto reply( State& state)
                     {
                        return [&state]( const common::message::transaction::commit::Reply& message)
                        {                                    
                           Trace trace{ "queue::forward::handle::transaction::commit::reply"};
                           common::log::debug( "message: ", message);

                           if( message.stage == decltype( message.stage)::prepare)
                              return; // we wait for the next message

                           auto pending = pending::consume( state.pending.transaction.commits, message.correlation);

                           if( ! pending)
                              return;

                           if( message.stage != decltype( message.stage)::commit)
                              common::log::line( common::log::category::error, message.state, "failed to commit - trid: ", message.trid);

                           state.forward_apply( pending->id, [&state]( auto& forward)
                           {
                              --forward.instances;

                              ++forward.metric.commit.count;
                              forward.metric.commit.last = platform::time::clock::type::now();

                              // we restart the "state machine"
                              detail::machine::next( state, forward);
                           });
                        };
                     }
                  } // commit
               } // transaction

               namespace state
               {
                  auto request( State& state)
                  {
                     return [&state]( const ipc::message::forward::group::state::Request& message)
                     { 
                        Trace trace{ "queue::forward::service::local::handle::state::request"};
                        log::debug( "message: ", message);

                        auto basic_assign = []( const auto& source, auto& target)
                        {
                           target.alias = source.alias;
                           target.source = source.source.queue;
                           target.instances.configured = source.instances.configured;
                           target.instances.running = source.instances.running;
                           target.metric.commit.count = source.metric.commit.count;
                           target.metric.commit.last = source.metric.commit.last;
                           target.metric.rollback.count = source.metric.rollback.count;
                           target.metric.rollback.last = source.metric.rollback.last;
                           target.note = source.note;
                           target.enabled = source.instances.enabled;
                        };

                        auto transform_service = [basic_assign]( const auto& service)
                        {
                           ipc::message::forward::group::state::Reply::Service result;
                           basic_assign( service, result);
                           result.target.service = service.target.service;

                           if( service.reply)
                           {
                              ipc::message::forward::group::state::Reply::Service::Reply reply;
                              reply.queue = service.reply.value().queue;
                              reply.delay = service.reply.value().delay;
                              result.reply = std::move( reply);
                           }

                           return result;
                        };

                        auto transform_queue = [basic_assign]( const auto& queue)
                        {
                           ipc::message::forward::group::state::Reply::Queue result;
                           basic_assign( queue, result);

                           result.target.delay = queue.target.delay;
                           result.target.queue = queue.target.queue;

                           return result;
                        };

                        auto reply = common::message::reverse::type( message, process::handle());
                        reply.alias = state.alias;
                        reply.services = algorithm::transform( state.forward.services, transform_service);
                        reply.queues = algorithm::transform( state.forward.queues, transform_queue);

                        state.multiplex.send( message.process.ipc, reply);
                     };
                  }
               } // state

               auto shutdown( State& state)
               {
                  return [&state]( const common::message::shutdown::Request& message)
                  {
                     Trace trace{ "queue::forward::service::local::handle::shutdown"};
                     log::debug( "message: ", message);

                     state.runlevel = decltype( state.runlevel())::shutdown;

                     // configure zero instances for all forwards
                     auto set_no_instances = []( auto& forward){ forward.instances.configured = 0;};
                     algorithm::for_each( state.forward.services, set_no_instances);
                     algorithm::for_each( state.forward.queues, set_no_instances);

                     local::comply::to::state( state);
                  };
               }
               
            } // handle

            auto handlers( State& state)
            {
               return message::dispatch::handler( ipc::device(),
                  common::message::dispatch::handle::defaults( state),

                  handle::configuration::update::request( state),
                  handle::state::request( state),
                  handle::queue::lookup::reply( state),
                  handle::queue::lookup::discard::reply( state),
                  handle::dequeue::reply( state),
                  handle::dequeue::forget::request( state),
                  handle::dequeue::forget::reply( state),
                  handle::service::lookup::reply( state),
                  handle::service::lookup::discard::reply( state),
                  handle::service::call::reply( state),
                  handle::enqueue::reply( state),
                  handle::transaction::commit::reply( state),
                  handle::transaction::rollback::reply( state),
                  handle::shutdown( state)
               );
            }

         } // <unnamed>
      } // local

      handler_type handlers( State& state)
      {
         return local::handlers( state);
      }

   } // queue::forward
} // casual
