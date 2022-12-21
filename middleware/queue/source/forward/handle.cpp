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
#include "common/execution.h"

namespace casual
{
   using namespace common;

   namespace queue::forward
   {
      namespace local
      {
         namespace
         {
            namespace transform::forward
            {

               auto service()
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
                     return result;
                  };
               }
               auto queue()
               {
                  return []( auto& queue)
                  {
                     state::forward::Queue result;
                     result.alias = queue.alias;
                     result.source.queue = queue.source;

                     result.target.queue = queue.target.queue;
                     result.target.delay = queue.target.delay;
                     
                     result.instances.configured = queue.instances;
                     return result;
                  };
               } 
            } // transform::forward

            namespace queue::lookup
            {

               template< typename M>
               auto assign( M&& message, state::forward::Service& forward)
               {
                  if( forward.source.queue == message.name)
                  {
                     if( message.remote())
                        common::event::error::send( code::casual::invalid_configuration, "can't forward from a remote queue: ", message.name);
                     else
                     {
                        forward.source.id = message.queue;
                        forward.source.process = message.process;
                     }
                  }

                  if( forward.reply)
                  {
                     auto& reply = forward.reply.value();

                     if( reply.queue == message.name)
                     {
                        reply.id = message.queue;
                        reply.process = message.process;
                     }
                  }

                  return forward.source && ( ! forward.reply || forward.reply.value());
               }

               template< typename M>
               auto assign( M&& message, state::forward::Queue& forward)
               {
                  if( forward.source.queue == message.name)
                  {
                     if( message.remote())
                        common::event::error::send( code::casual::invalid_configuration, "can't forward from a remote queue: ", message.name);
                     else
                     {
                        forward.source.id = message.queue;
                        forward.source.process = message.process;
                     }
                  }

                  if( forward.target.queue == message.name)
                  {
                     forward.target.id = message.queue;
                     forward.target.process = message.process;
                  }

                  return forward.source && forward.target;
               }

               namespace detail
               {
                  namespace send
                  {
                     template< typename ID>
                     auto request( State& state, ID id, const std::string& name)
                     {
                        ipc::message::lookup::Request request{ process::handle()};
                        request.name = name;
                        request.context.semantic = decltype( request.context.semantic)::wait;

                        common::log::line( verbose::log, "request: ", request);

                        state::pending::queue::Lookup pending;
                        pending.id = id;
                        pending.correlation = state.multiplex.send( ipc::queue::manager(), request);
                        return pending;
                     };
                  } // send
                  

                  void request( State& state, state::forward::Service& forward)
                  {
                     state.pending.queue.lookups.push_back( detail::send::request( state, forward.id, forward.source.queue));

                     if( forward.reply)
                        state.pending.queue.lookups.push_back( detail::send::request( state, forward.id, forward.reply.value().queue));
                  }


                  void request( State& state, state::forward::Queue& forward)
                  {
                     state.pending.queue.lookups.push_back( detail::send::request( state, forward.id, forward.source.queue));
                     state.pending.queue.lookups.push_back( detail::send::request( state, forward.id, forward.target.queue));
                  }

               } // detail

               auto request( State& state)
               {
                  return [&state]( auto& forward)
                  {
                     detail::request( state, forward);
                  };
               }
               
            } // queue::lookup

            namespace pending
            {
               template< typename P>
               auto consume( P& pending, const strong::correlation::id& correlation)
               {
                  auto found = common::algorithm::find( pending, correlation);
                  assert( found);

                  return algorithm::container::extract( pending, std::begin( found));
               }
            } // pending

            namespace send
            {
               namespace dequeue
               {
                  template< typename F>
                  void request( State& state, F& forward)
                  {
                     Trace trace{ "queue::forward::send::dequeue::request"};
                     common::log::line( verbose::log, "forward: ", forward);

                     auto send_request =[&]()
                     {
                        ipc::message::group::dequeue::Request request{ common::process::handle()};
                        request.trid = common::transaction::id::create( common::process::handle());
                        request.correlation = strong::correlation::id::emplace( common::uuid::make());
                        request.queue = forward.source.id;
                        request.name = forward.source.queue;
                        request.selector = forward.selector;
                        request.block = true;

                        using Pending = common::traits::remove_cvref_t< decltype( state.pending.dequeues.back())>;

                        Pending pending;
                        pending.id = forward.id;
                        pending.correlation = request.correlation;
                        pending.trid = request.trid;

                        state.pending.dequeues.push_back( std::move( pending));
                        ++forward;

                        state.multiplex.send( forward.source.process.ipc, request, [ &state]( auto& ipc, auto& complete)
                        {
                           algorithm::container::erase( state.pending.dequeues, complete.correlation());
                           // TODO send "error reply"?

                        });
                        
                     };

                     common::algorithm::for_n( forward.instances.missing(), send_request);
                  }
               } // dequeue

               namespace transaction
               {
                  namespace rollback
                  {
                     template< typename P>
                     void request( State& state, P&& pending)
                     {
                        Trace trace{ "queue::forward::send::transaction::rollback::request"};
                        common::log::line( verbose::log, "pending: ", pending);

                        common::message::transaction::rollback::Request request{ common::process::handle()};
                        request.correlation = pending.correlation;
                        request.trid = pending.trid;

                        state.multiplex.send( ipc::transaction::manager(), request);

                        state.pending.transaction.rollbacks.emplace_back( std::forward< P>( pending));
                     }
                  } // rollback

                  namespace commit
                  {
                     template<  typename P>
                     void request( State& state, P&& pending)
                     {
                        Trace trace{ "queue::forward::send::transaction::commit::request"};
                        common::log::line( verbose::log, "pending: ", pending);

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
                  
                  state.multiplex.send( forward.source.process.ipc, request);
               }

               template< typename F>
               void request( State& state, const F& forward, const state::pending::service::Lookup& pending)
               {
                  message::service::lookup::discard::Request request{ process::handle()};
                  request.correlation = pending.correlation;
                  request.requested = forward.target.service;

                  state.multiplex.send( ipc::service::manager(), request);
               }
            } // detail::send::discard

            namespace comply::to
            {
               void state( State& state)
               {
                  Trace trace{ "queue::forward::service::local::comply::to::state"};

                  // take care of pending lookups
                  {
                     Trace trace{ "queue::forward::service::local::comply::to::state send_discard_request"};

                     auto send_discard_request = [ &state]( auto& pending)
                     {
                        ipc::message::lookup::discard::Request request{ process::handle()};
                        request.correlation = pending.correlation;
                        state.multiplex.send( ipc::queue::manager(), request);
                     };
                     algorithm::for_each( state.pending.queue.lookups, send_discard_request);
                  }

                  // Forwards will wait forever for dequeues and service lookups, so we cancel enough
                  // to ensure that enough flows will reach a final state
                  {
                     auto cancel_surplus_instances = [ &state]( const auto& forward)
                     {
                        auto surplus = forward.instances.surplus();
                        if( surplus == 0)
                           return;

                        // Pendings for a given forward
                        auto get_pendings = [ &forward]( auto& pendings)
                        {
                           return algorithm::filter( pendings, [ &forward]( const auto& pending){ return pending.id == forward.id;});
                        };

                        auto send_discard_request = [ &state, &forward, &surplus]( const auto& pending)
                        {
                           detail::send::discard::request( state, forward, pending);
                           return --surplus > 0;
                        };

                        algorithm::for_each_while( get_pendings( state.pending.dequeues), send_discard_request);

                        if constexpr( std::is_same_v< traits::remove_cvref_t< decltype( forward)>, state::forward::Service>)
                           algorithm::for_each_while( get_pendings( state.pending.service.lookups), send_discard_request);
                     };

                     algorithm::for_each( state.forward.queues, cancel_surplus_instances);
                     algorithm::for_each( state.forward.services, cancel_surplus_instances);
                  }

                  // start the "flows" for configured forwards
                  auto lookup_request = []( auto& state, auto& forwards)
                  {
                     auto is_configured = []( auto& forward)
                     {
                        return forward.instances.configured > 0;
                     };

                     algorithm::for_each( 
                        algorithm::filter( forwards, is_configured), 
                        local::queue::lookup::request( state));

                  };

                  // start lookups on queues, even if we got them already.
                  lookup_request( state, state.forward.services);
                  lookup_request( state, state.forward.queues);

                  log::line( verbose::log, "state: ", state);

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
                        log::line( verbose::log, "message: ", message);

                        state.alias = message.model.alias;

                        // TODO maintenance we only update instances
                        auto add_or_update = []( auto& source, auto& target, auto transform)
                        {
                           auto handle_source = [&]( auto& forward)
                           {
                              auto is_alias = [&alias = forward.alias]( auto& forward){ return forward.alias == alias;};

                              if( auto found = algorithm::find_if( target, is_alias))
                                 found->instances.configured = forward.instances;
                              else
                                 target.push_back( transform( forward));
                           };

                           algorithm::for_each( source, handle_source);
                        };

                        add_or_update( message.model.services, state.forward.services, transform::forward::service());
                        add_or_update( message.model.queues, state.forward.queues, transform::forward::queue());

                        comply::to::state( state);

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
                        log::line( verbose::log, "message: ", message);

                        auto pending = pending::consume( state.pending.queue.lookups, message.correlation);
                        log::line( verbose::log, "pending: ", pending);

                        if( state.runlevel > decltype( state.runlevel())::running)
                           return;

                        state.forward_apply( pending.id, [&]( auto& forward)
                        {
                           // if the forward has it's queues we start the 'flow'
                           if( local::queue::lookup::assign( message, forward))
                              send::dequeue::request( state, forward);
                        });                               
                     };
                  }
                  namespace discard
                  {
                     auto reply( State& state)
                     {
                        return [&state]( const ipc::message::lookup::discard::Reply& message)
                        {
                           Trace trace{ "queue::forward::handle::queue::lookup::discard::reply"};
                           common::log::line( verbose::log, "message: ", message);

                           if( message.state == decltype( message.state)::replied)
                              return; // we've received the queue already, and we let the 'flow' do it's thing...

                           auto pending = pending::consume( state.pending.queue.lookups, message.correlation);
                           common::log::line( verbose::log, "pending: ", pending);

                           // we don't need to do anything, since the forward instance can't possible be 
                           // started if we're waiting for lookup discards

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
                        log::line( verbose::log, "message: ", message);

                        auto pending = pending::consume( state.pending.dequeues, message.correlation);
                        
                        if( message.message.empty())
                        {
                           log::line( log::category::error, "empty dequeued message - action: rollback");
                           send::transaction::rollback::request( state, std::move( pending));
                        }
                        else if( state.runlevel > decltype( state.runlevel())::running)
                        {
                           // if we're in _shutdown mode_ we don't want to start any "flows"
                           log::line( log::category::information, "message dequeued in shutdown mode - the message might end up on error queue - action: rollback");
                           send::transaction::rollback::request( state, std::move( pending));
                        }
                        else if( auto forward = state.forward_service( pending.id))
                        {

                           // request service
                           message::service::lookup::Request request{ process::handle()};
                           request.correlation = pending.correlation;
                           request.requested = forward->target.service;
                           // we'll wait 'forever'
                           request.context.semantic = decltype( request.context.semantic)::wait;

                           state.multiplex.send( ipc::service::manager(), request);

                           forward::state::pending::service::Lookup lookup{ std::move( pending)};
                           lookup.buffer.type = std::move( message.message[ 0].payload.type);
                           lookup.buffer.data = std::move( message.message[ 0].payload.data);

                           state.pending.service.lookups.push_back( std::move( lookup));

                           log::line( verbose::log, "state.pending.service.lookups: ", state.pending.service.lookups);
                        }
                        else if( auto forward = state.forward_queue( pending.id))
                        {
                           ipc::message::group::enqueue::Request request{ process::handle()};
                           request.correlation = pending.correlation;
                           request.trid = pending.trid;
                           request.queue = forward->target.id;
                           request.name = forward->target.queue;
                           request.message = std::move( message.message.front());

                           // make sure we've got a new message-id
                           request.message.id = uuid::make();

                           
                           if( forward->target.delay > platform::time::unit::zero())
                              request.message.attributes.available = platform::time::clock::type::now() + forward->target.delay;

                           log::line( verbose::log, "enqueue request: ", request);

                           state.multiplex.send( forward->target.process.ipc, request);
                           state.pending.enqueues.emplace_back( std::move( pending));

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

                           if( auto found = algorithm::find( state.pending.dequeues, message.correlation))
                           {
                              common::log::line( verbose::log, "found: ", *found);

                              auto pending = algorithm::container::extract( state.pending.dequeues, std::begin( found));

                              state.forward_apply( pending.id, []( auto& forward)
                              {
                                 --forward;                                 
                                 common::log::line( verbose::log, "forward: ", forward);
                              });

                              common::log::line( verbose::log, "state.pending.dequeues: ", state.pending.dequeues);
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
                           common::log::line( verbose::log, "message: ", message);

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
                           common::log::line( verbose::log, "message: ", message);

                           if( message.discarded)
                           {
                              // If a lookup was discarded, we should be find a matching pending
                              auto pending = pending::consume( state.pending.dequeues, message.correlation);

                              state.forward_apply( pending.id, [ &state]( auto& forward)
                              {
                                 --forward;
                                 common::log::line( verbose::log, "forward: ", forward);

                                 // We try to restart the flow
                                 send::dequeue::request( state, forward);
                              });
                           }

                           // if not found, we've already got a dequeue, and we're mid 'state flow'.
                           // the forward has been configured already, and we do nothing and let the
                           // flow end/die of natural causes... 

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
                           log::line( verbose::log, "message: ", message);

                           if( message.state == decltype( message.state)::busy)
                              return;


                           auto pending = pending::consume( state.pending.service.lookups, message.correlation);

                           if( message.state != decltype( message.state)::idle)
                           {
                              log::line( log::category::error, "service not callable: ", message.service.name);
                              send::transaction::rollback::request( state, std::move( pending));
                              return;
                           }

                           // Generate a new execution id for each service call
                           common::execution::reset();

                           message::service::call::caller::Request request{ pending.buffer};
                           request.process = process::handle();
                           request.correlation = pending.correlation;
                           request.trid = pending.trid;
                           request.service = std::move( message.service);

                           state.multiplex.send( message.process.ipc, request);
                           state.pending.service.calls.emplace_back( std::move( pending), message.process.pid);
                        };
                     }

                     namespace discard
                     {
                        auto reply( State& state)
                        {
                           return [&state]( const message::service::lookup::discard::Reply& message)
                           {
                              Trace trace{ "queue::forward::service::local::handle::service::lookup::discard::reply"};
                              log::line( verbose::log, "message: ", message);

                              if( message.state == decltype( message.state)::replied)
                                 return; // we've got the lookup reply already, let the 'flow' do it's thing...

                              auto pending = pending::consume( state.pending.service.lookups, message.correlation);
                              log::line( verbose::log, "pending: ", pending);

                              // rollback the 'flow'
                              send::transaction::rollback::request( state, std::move( pending));
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
                           log::line( verbose::log, "message: ", message);

                           auto call = pending::consume( state.pending.service.calls, message.correlation);

                           if( message.code.result != code::xatmi::ok)
                           {
                              send::transaction::rollback::request( state, std::move( call));
                              return;
                           }

                           // we know that this is a service forward
                           auto forward = state.forward_service( call.id);
                           assert( forward);

                           if( forward->reply)
                           {
                              auto& reply = forward->reply.value();
                              ipc::message::group::enqueue::Request request{ process::handle()};
                              request.correlation = call.correlation;
                              request.trid = call.trid;
                              request.queue = reply.id;
                              request.name = reply.queue;
                              request.message.payload.type = std::move( message.buffer.type);
                              request.message.payload.data = std::move( message.buffer.data);

                              if( reply.delay > platform::time::unit::zero())
                                 request.message.attributes.available = platform::time::clock::type::now() + reply.delay;

                              log::line( verbose::log, "enqueue reply: ", request);
                              
                              state.pending.enqueues.emplace_back( std::move( call));
                              state.multiplex.send( reply.process.ipc, request, [ &state]( auto& ipc, auto& complete)
                              {
                                 // remove the added pending.
                                 auto pending = pending::consume( state.pending.enqueues, complete.correlation());
                                 
                                 send::transaction::rollback::request( state, std::move( pending));
                              });
                           }
                           else
                              send::transaction::commit::request( state, std::move( call));

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
                        common::log::line( verbose::log, "message: ", message);

                        auto pending = pending::consume( state.pending.enqueues, message.correlation);

                        send::transaction::commit::request( state, std::move( pending));
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
                           common::log::line( verbose::log, "message: ", message);

                           auto pending = pending::consume( state.pending.transaction.rollbacks, message.correlation);

                           if( message.stage != decltype( message.stage)::rollback)
                              common::log::line( common::log::category::error, message.state, "failed to rollback - trid: ", message.trid);


                           state.forward_apply( pending.id, [&state]( auto& forward)
                           {
                              --forward;
                              ++forward.metric.rollback.count;
                              forward.metric.rollback.last = platform::time::clock::type::now();

                              // we try to dequeue again... 
                              send::dequeue::request( state, forward);
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
                           common::log::line( verbose::log, "message: ", message);

                           if( message.stage == decltype( message.stage)::prepare)
                              return; // we wait for the next message

                           auto pending = pending::consume( state.pending.transaction.commits, message.correlation);

                           if( message.stage != decltype( message.stage)::commit)
                              common::log::line( common::log::category::error, message.state, "failed to commit - trid: ", message.trid);

                           state.forward_apply( pending.id, [&state]( auto& forward)
                           {
                              --forward;

                              ++forward.metric.commit.count;
                              forward.metric.commit.last = platform::time::clock::type::now();

                              // we try to dequeue again... 
                              send::dequeue::request( state, forward);
                           });
                        };
                     }
                  } // rollback
               } // transaction

               namespace state
               {
                  auto request( State& state)
                  {
                     return [&state]( const ipc::message::forward::group::state::Request& message)
                     { 
                        Trace trace{ "queue::forward::service::local::handle::state::request"};
                        log::line( verbose::log, "message: ", message);

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

               namespace dead
               {
                  namespace detail::transform
                  {
                     auto dequeue( forward::state::pending::Dequeue& pending)
                     {
                        ipc::message::group::dequeue::forget::Request message{ common::process::handle()};
                        message.correlation = pending.correlation;
                        return message;
                     }

                     auto enqueue( forward::state::pending::Enqueue& pending)
                     {
                        ipc::message::group::enqueue::Reply message;
                        message.correlation = pending.correlation;
                        return message;
                     }

                  } // detail::transform

                  auto process( State& state)
                  {
                     return [ &state]( const common::message::event::process::Exit& message)
                     {
                        Trace trace{ "queue::forward::service::local::handle::dead::process"};
                        log::line( verbose::log, "message: ", message);

                        // Note: We rely on others to send us "error replies" for the following 
                        //  * pending.service.calls : service-manager
                        //  * pending.transaction.(commit|rollback) : transaction-manager

                        // ids of all logical instances that interact with the `pid` in some
                        // way ( enqueue, dequeue, _target enqueue_, _reply enqueue_).
                        auto ids = state.forward.ids( message.state.pid);
                        log::line( verbose::log, "ids: ", ids);

                        // emulates the messages/replies, and let the _normal flow_ take care of business.
                        auto emulate_error_reply = []( auto& pendings, auto& ids, auto transformer)
                        {
                           auto invalid = std::get< 0>( algorithm::intersection( pendings, ids));
                           log::line( verbose::log, "invalid: ", invalid);

                           algorithm::for_each( algorithm::transform( invalid, transformer), []( auto& message)
                           {
                              ipc::device().push( std::move( message));
                           });

                           algorithm::container::erase( pendings, invalid);
                        };

                        emulate_error_reply( state.pending.dequeues, ids, &detail::transform::dequeue);
                        emulate_error_reply( state.pending.enqueues, ids, &detail::transform::enqueue);
 
                        
                        state.invalidate( ids);
                     };                     
                  }

               } // dead

               auto shutdown( State& state)
               {
                  return [&state]( const common::message::shutdown::Request& message)
                  {
                     Trace trace{ "queue::forward::service::local::handle::shutdown"};
                     log::line( verbose::log, "message: ", message);

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

                  common::event::listener( handle::dead::process( state)),
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
