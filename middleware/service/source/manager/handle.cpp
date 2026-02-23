//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "service/manager/handle.h"
#include "service/manager/admin/server.h"
#include "service/manager/transform.h"
#include "service/manager/configuration.h"
#include "service/common.h"


#include "common/signal/timer.h"
#include "common/environment.h"
#include "common/environment/normalize.h"
#include "common/algorithm.h"
#include "common/algorithm/is.h"
#include "common/algorithm/coalesce.h"
#include "common/process.h"
#include "common/message/dispatch.h"
#include "common/message/dispatch/handle.h"
#include "common/message/internal.h"
#include "common/event/listen.h"
#include "common/event/send.h"
#include "common/service/type.h"

#include "common/communication/instance.h"

#include "configuration/message.h"
#include "configuration/model/change.h"

#include "domain/discovery/api.h"

#include "casual/assert.h"

// std
#include <vector>
#include <string>

namespace casual
{
   namespace service::manager::handle
   {
      namespace ipc
      {
         common::communication::ipc::inbound::Device& device()
         {
            return common::communication::ipc::inbound::device();
         }
      } // ipc

      namespace local
      {
         namespace
         {
            auto set_timer( const state::service::pending::deadline::Directive& directive, common::chronology::time_point now = platform::time::clock::type::now())
            {
               if( auto time_point = std::get_if< common::chronology::time_point>( &directive))
                  common::signal::timer::set( *time_point - now);
               else if( std::get_if< state::service::pending::deadline::Unset>( &directive))
                  common::signal::timer::unset();
            }

            std::string service_name( const State& state, state::service::id::type service_id)
            {
               if( state.services.contains( service_id))
                  return state.services[ service_id].information.logical_name();
               else
                  return "<unknown>";
            }

            std::string instance_alias( const State& state, state::instance::sequential::id::type instance_id)
            {
               if( state.instances.sequential.contains( instance_id))
                  return state.instances.sequential[ instance_id].alias;
               else
                  return "<unknown>";
            }
            


            namespace optional
            {
               template< typename D, typename M>
               auto send( State& state, D&& device, M&& message)
               {
                  common::log::debug( "send message: ", message);
                  return state.multiplex.send( device, message);
               }
            } // optional

            namespace error
            {
               auto reply( State& state, const state::instance::Reservation& reservation, common::code::xatmi code)
               {
                  Trace trace{ "service::manager::handle::local::error::reply"};
                  common::log::debug( "caller: ", reservation.caller, ", code: ", code, ", service: ", reservation.caller.service);

                  common::message::service::call::Reply message;
                  message.correlation = reservation.caller.correlation;
                  message.code.result = code; 

                  state.multiplex.send( reservation.caller.process.ipc, std::move( message));

                  if( state.events.active< common::message::event::service::Calls>())
                  {
                     common::message::event::service::Metric metric;
                     metric.code.result = code;
                     metric.correlation = reservation.caller.correlation;
                     metric.service = state.services[ reservation.caller.service].information.logical_name();
                     metric.process = reservation.callee;
                     metric.trid = reservation.caller.trid;

                     state.metric.add( std::move( metric));
                     handle::metric::batch::send( state);
                  }
               }
            } // error

            namespace lookup
            {
               auto timeout( State& state, state::service::pending::Lookup lookup)
               {
                  Trace trace{ "service::manager::handle::local::lookup::timeout"};
                  common::log::debug( "lookup: ", lookup);

                  auto reply = common::message::reverse::type( lookup.request);
                  reply.state = decltype( reply.state)::timeout;
                  state.multiplex.send( lookup.request.process.ipc, std::move( reply));
               }
            } // lookup

            namespace metric
            {
               void send( State& state)
               {
                  state.events( state.multiplex, state.metric.extract());
               }
            } // metric

            namespace discovery
            {
               auto send( State& state, std::vector< std::string> services, const common::strong::correlation::id& correlation = common::strong::correlation::id::generate())
               {
                  Trace trace{ "service::manager::handle::local::discovery::send"};
                  return casual::domain::discovery::request( state.multiplex, std::move( services), {}, correlation);
               }
            }

         } // <unnamed>
      } // local

      void timeout( State& state)
      {
         Trace trace{ "service::manager::handle::timeout"};

         const auto now = platform::time::clock::type::now();

         auto expired = state.pending.deadline.expired( now);
         common::log::debug( "expired: ", expired);

         auto handle_timeout = [&state]( auto& entry)
         {
            auto order_assassination = []( State& state, auto& entry, auto instance_id)
            {
               auto& service = state.services[ entry.service];

               auto contract = service.timeout.contract.value_or( common::service::execution::timeout::contract::Type::linger);
               auto announcement = common::string::compose( "service ", service.information.name, " timed out");

               // if the contract is fatal we need to 'disable' the instance. This to mitigate
               // the possibility of we get an ack from the instance and we've got pending lookups
               // on services that the instance has -> we reserve it, and it gets killed. 
               // Hence, we need to know so we don't reserve instances that we know are going to
               // get killed.
               if( common::service::execution::timeout::contract::fatal( contract))
                  state.disabled.push_back( instance_id);

               // send event, at least domain-manager want's to know...
               common::message::event::process::Assassination event{ common::process::handle()};
               event.target = state.instances.sequential[ instance_id].process.pid;
               event.contract = contract;
               event.announcement = announcement;
               common::event::send( event);
            };

            // check if it's a lookup that has timed out.
            if( auto found = common::algorithm::find( state.pending.lookups, entry.correlation))
            {
               local::lookup::timeout( state, common::algorithm::container::extract( state.pending.lookups, std::begin( found)));
               return;
            }

            if( ! state.services.contains( entry.service))
            {
               common::log::error( common::code::casual::invalid_semantics, "timeout entry has no associated service: ", entry);
               return;
            }

            // we're ready to Assassinate

            if( state.instances.sequential.contains( entry.target))
            {
               auto& instance = state.instances.sequential[ entry.target];

               if( auto caller = instance.consume( entry.correlation))
               {
                  // keep track of instance until we get an ACK, or the server dies
                  // We need to notify TM if this call was in transaction.
                  state.timeout_instances.push_back( instance.process.pid);

                  // we only send error reply if the caller wants one.
                  if( caller.semantic == state::instance::caller::Semantic::reply)
                     local::error::reply( state, { .caller = caller, .callee = instance.process}, common::code::xatmi::timeout);
                  
                  order_assassination( state, entry, entry.target);
               }
               else
               {
                  common::log::error( common::code::casual::invalid_semantics, "failed to consume caller from timeout entry - target.alias: ", 
                     local::instance_alias( state, entry.target), ", target.service: ", local::service_name( state, entry.service), ", entry: ", entry);
               }

            }
            else
            {
               common::log::error( common::code::casual::invalid_semantics, "timeout entry has invalid target: ", entry);
            }

         };

         common::algorithm::for_each( expired.entries, handle_timeout);

         if( expired.deadline)
            local::set_timer( *expired.deadline, now);
      }

      namespace metric
      {
         void send( State& state)
         {
            if( state.metric)
               local::metric::send( state);
         }

         namespace batch
         {
            void send( State& state)
            {
               if( state.metric.size() >= platform::batch::service::metrics)
                  local::metric::send( state);
            }
         } // batch
      } // metric

      namespace process
      {
         void exit( const common::process::lifetime::Exit& exit)
         {
            // We put a dead process event on our own ipc device, that
            // will be handled later on.
            common::message::event::process::Exit event{ exit};
            common::communication::ipc::inbound::device().push( std::move( event));
         }
      } // process

      namespace local
      {
         namespace
         {
            namespace service::detail::handle
            {
               // defined after lookup, below
               void pending( State& state, std::vector< state::service::pending::Lookup> pending);
               void pending( State& state, State::update_result_t update);
                  
            } // service::detail::handle

            namespace detail
            {
               void check_timeout_and_notify_TM( State& state, common::strong::process::id pid, common::transaction::global::id::range gtrid)
               {
                  Trace trace{ "service::manager::handle::local::detail::check_timeout_and_notify_TM"};

                  common::log::debug( "state.timeout_instances: ", state.timeout_instances);
                  
                  if( auto found = common::algorithm::find( state.timeout_instances, pid))
                  {
                     state.timeout_instances.erase( std::begin( found));

                     if( gtrid.empty())
                        return;

                     // notify TM about the potential stale transaction.
                     common::message::transaction::potential::Stale message{ common::process::handle()};
                     message.gtrid = common::transaction::global::ID{ gtrid};
                     optional::send( state, common::communication::instance::outbound::transaction::manager::device(), message);
                  }
               }
               
            } // detail

            namespace event
            { 
               namespace process
               {
                  auto exit( State& state)
                  {
                     return [&state]( common::message::event::process::Exit& event)
                     {
                        Trace trace{ "service::manager::handle::local::event::process::detail::exit"};
                        common::log::debug( "event: ", event);

                        state.pending.shutdown.failed( event.state.pid);

                        auto removed = state.remove( event.state.pid);

                        if( removed.deadline)
                           local::set_timer( *removed.deadline);

                        // we need to check if the dead process has anyone waiting for a reply
                        for( auto reservation : removed.reservations)
                        {
                           common::log::error( common::code::casual::invalid_semantics, " callee terminated with pending reply to caller - callee: ", 
                                 event.state.pid, " - caller: ", reservation.caller.process.pid);

                           local::error::reply( state, reservation, common::code::xatmi::service_error);

                           // we might need to notify TM about a potential stale transaction
                           detail::check_timeout_and_notify_TM( state, event.state.pid, reservation.caller.trid.global());
                        }

                        // It might be an assassinated instance. This should be taken care of by check_timeout_and_notify_TM
                        // but just to be sure we do it here as well.
                        common::algorithm::container::erase( state.timeout_instances, event.state.pid);

                        // The dead process might be the last instance that supplied a given set of services. 
                        // We need to check pending lookups... We just "invalidate and re-lookup"
                        service::detail::handle::pending( state, std::exchange( state.pending.lookups, {}));
                     };
                  }
               } // process

               namespace transaction
               {
                  auto disassociate( State& state)
                  {
                     return [ &state]( const common::message::event::transaction::Disassociate& message)
                     {
                        Trace trace{ "service::manager::handle::local::event::transaction::disassociate"};
                        common::log::debug( "message: ", message);

                        auto instances = state.disassociate( message.gtrid.range());

                        common::log::debug( "disassociated instances: ", instances);
                     };
                  }
                  
               } // transaction

               namespace subscription
               {
                  auto begin( State& state)
                  {
                     return [&state]( common::message::event::subscription::Begin& message)
                     {
                        Trace trace{ "service::manager::handle::event::subscription::Begin"};
                        common::log::debug( "message: ", message);

                        state.events.subscription( message);
                     };
                  }

                  auto end( State& state)
                  {
                     return [&state]( common::message::event::subscription::End& message)
                     {
                        Trace trace{ "service::manager::handle::event::subscription::End"};
                        common::log::debug( "message: ", message);

                        state.events.subscription( message);
                     };
                  }
               } // subscription

            } // event

            namespace service
            {
               auto advertise( State& state)
               {
                  return [&state]( common::message::service::Advertise& message)
                  {
                     Trace trace{ "service::manager::handle::service::advertise"};
                     common::log::debug( "message: ", message);

                     // some pending might got resolved, from the update
                     detail::handle::pending( state, state.update( std::move( message)));
                  };
               }

               namespace concurrent
               {
                  auto advertise( State& state)
                  {
                     return [&state]( common::message::service::concurrent::Advertise& message)
                     {
                        Trace trace{ "service::manager::handle::service::concurrent::advertise"};
                        common::log::debug( "message: ", message);

                        // some pending might got resolved.
                        detail::handle::pending( state, state.update( std::move( message)));
                     };
                  }

                  auto metric( State& state)
                  {
                     return [ &state]( common::message::event::service::Calls& message)
                     {
                        Trace trace{ "service::manager::handle::service::concurrent::metric"};
                        common::log::debug( "message: ", message);

                        for( auto& metric : message.metrics)
                           state.services.metric( metric.service).update( metric);

                        if( state.events)
                        {
                           state.metric.add( std::move( message.metrics));
                           handle::metric::batch::send( state);
                        }
                     };
                  }
               } // concurrent

               namespace detail
               {
                  auto calculate_deadline( const state::Service& service, common::chronology::time_point now, std::optional< common::chronology::time_point> caller_deadline) -> std::optional< common::chronology::time_point>
                  {
                     if( service.timeout.duration && service.timeout.duration > std::chrono::microseconds{ 0})
                     {
                        auto deadline = now + *service.timeout.duration;
                        if( ! caller_deadline || *caller_deadline > deadline)
                           return { deadline};
                     }
                     return caller_deadline;
                  };

                  void discover( State& state, common::message::service::lookup::Request&& message, const std::string& name)
                  {
                     Trace trace{ "service::manager::handle::local::service::detail::discover"};

                     common::log::debug( "failed to find service: ", name, " - action: discover");

                     auto send_reply = common::execute::scope( [&]()
                     {
                        common::log::debug( "no instances found for service: ", name);

                        // Server that hosts the requested service is not found.
                        // We propagate this by having absent state
                        auto reply = common::message::reverse::type( message);
                        reply.service.name = message.requested;
                        reply.state = decltype( reply.state)::absent;

                        local::optional::send( state, message.process.ipc, reply);
                     });

                     {
                        common::log::debug( "no instances found for service: ", name, " - action: ask neighbor domains");

                        if( local::discovery::send( state, { name}, message.correlation) || message.context.semantic == decltype( message.context.semantic)::wait)
                        {
                           // we sent the request OR the caller is willing to wait for future 
                           // advertised services

                           state.pending.lookups.emplace_back( std::move( message), platform::time::clock::type::now());
                           send_reply.release();
                        }
                     }
                  }

                  namespace dispatch::lookup
                  {
                     void no_entry( State& state, common::message::service::lookup::Request& message)
                     {
                        common::log::debug( "failed to find service: ", message.requested, " - action: reply with ", common::code::xatmi::no_entry);
                        auto reply = common::message::reverse::type( message);
                        reply.service.name = message.requested;
                        reply.state = decltype( reply.state)::absent;

                        state.multiplex.send( message.process.ipc, reply);
                     }

                     void reply( State& state, state::service::id::type service_id, auto instance_id, common::message::service::lookup::Request& message, common::chronology::duration pending)
                     {
                        Trace trace{ "service::manager::handle::local::service::detail::dispatch::lookup::reply"};
                        common::log::debug( "'reserved' instance: ", instance_id);

                        static constexpr bool is_concurrent = std::same_as< state::instance::concurrent::id::type, decltype( instance_id)>;

                        auto destination = [ &state, instance_id]()
                        {
                           if constexpr( is_concurrent)
                              return state.instances.concurrent[ instance_id].process;
                           else
                              return state.instances.sequential[ instance_id].process;
                        }();

                        auto& service = state.services[ service_id];

                        auto reply = common::message::reverse::type( message);
                        reply.service = service.information;
                        reply.state = decltype( reply.state)::idle;
                        reply.process = destination;
                        reply.pending = pending;

                        if( service.information.name != message.requested)
                           reply.service.requested = message.requested;

                        const auto now = platform::time::clock::type::now();

                        if constexpr( is_concurrent)
                        {
                           if( auto deadline = detail::calculate_deadline( service, now, message.deadline))
                           {
                              common::log::debug( "deadline: ", deadline);
                              reply.deadline.remaining = *deadline - now;
                           }
                        }
                        else
                        {
                           // only 'sequential'/local instances might have pending lookup, and have a timeout associated
                           if( auto entry = state.pending.deadline.find_entry( message.correlation))
                           {
                              entry->service = service_id;
                              entry->target = instance_id;

                              // give caller the remaining duration of the deadline
                              reply.deadline.remaining = entry->when - now;
                           }
                           // otherwise, check if we need to set a new deadline.
                           else if( auto deadline = detail::calculate_deadline( service, now, message.deadline))
                           {
                              common::log::debug( "deadline: ", deadline);
                              common::log::debug( "service_id: ", service_id);
                           
                              // no pending, the caller get's the whole duration of the deadline.
                              reply.deadline.remaining = *deadline - now;

                              auto next = state.pending.deadline.add( {
                                 .when = *deadline,
                                 .correlation = message.correlation,
                                 .target = instance_id,
                                 .service = service_id});

                              if( next)
                                 local::set_timer( *next, now);
                           }
                        }

                        // send reply, if caller gone, we discard the reservation.
                        state.multiplex.send( message.process.ipc, reply, [ &state]( auto& destination, auto& complete)
                        {
                           if( auto instance_id = state.instances.sequential.lookup( destination))
                              state.instances.sequential[ instance_id].discard();
                        });
                     }

                     void pending( State& state, state::service::id::type service_id, common::message::service::lookup::Request&& message)
                     {
                        switch( message.context.semantic)
                        {
                           using Semantic = decltype( message.context.semantic);

                           case Semantic::no_reply:
                           {
                              auto reply = common::message::reverse::type( message);
                              reply.service = state.services[ service_id].information;

                              // The intention is "send and forget", or a plain forward, we use our forward-cache for this
                              reply.process = state.forward;

                              // Caller will think that service is idle, that's the whole point
                              // with our forward.
                              reply.state = decltype( reply.state)::idle;

                              local::optional::send( state, message.process.ipc, reply);
                              break;
                           }
                           case Semantic::forward_request:
                              // This is a request from service-forward from a previous _forward_ lookup.
                              // We treat it as "regular" pending lookup.
                              [[fallthrough]];
                           case Semantic::regular:
                           {
                              auto now = platform::time::clock::type::now();

                              if( auto deadline = detail::calculate_deadline( state.services[ service_id], now, message.deadline))
                              {
                                 if( state.services[ service_id].has_sequential())
                                 {
                                    auto next = state.pending.deadline.add( { 
                                       .when = *deadline, 
                                       .correlation = message.correlation,
                                       .service = service_id});

                                    if( next)
                                       local::set_timer( *next, now);
                                 }
                              }

                              state.pending.lookups.emplace_back( std::move( message), now);

                              break;
                           }
                           case Semantic::wait:
                           {
                              // we know the service exists, and it got instances, we do a regular pending.
                              state.pending.lookups.emplace_back( std::move( message), platform::time::clock::type::now());

                              break;
                           }
                        }
                     }

                     bool internal_only( State& state, state::service::id::type service_id, common::message::service::lookup::Request& message, common::chronology::duration pending)
                     {
                        Trace trace{ "service::manager::handle::local::service::detail::dispatch::lookup::internal_only"};

                        if( state.services[ service_id].has_sequential())
                        {
                           auto get_caller = [ service_id]( const auto& message) -> state::instance::Caller
                           {
                              auto semantic = message.no_reply() ? state::instance::caller::Semantic::no_reply : state::instance::caller::Semantic::reply;

                              return { .process = message.process, .correlation = message.correlation, .trid = message.trid, .service = service_id, .semantic = semantic};
                           };

                           if( auto instance_id = state.reserve_sequential( get_caller( message)))
                              dispatch::lookup::reply( state, service_id, instance_id, message, pending);
                           else
                              dispatch::lookup::pending( state, service_id, std::move( message));
                           return true;
                        }
                        return false;
                     }

                     bool external_internal( State& state, state::service::id::type service_id, common::message::service::lookup::Request& message, common::chronology::duration pending)
                     {
                         Trace trace{ "service::manager::handle::local::service::detail::dispatch::lookup::external_internal"};

                        if( dispatch::lookup::internal_only( state, service_id, message, pending))
                           return true;

                        if( ! message.trid)
                        {
                           if( auto instance_id = state.reserve_concurrent( service_id, {}))
                           {
                              dispatch::lookup::reply( state, service_id, instance_id, message, pending);
                              return true;
                           }
                           return false;
                        }

                        // check if the gtrid has associations before
                        if( auto found = common::algorithm::find( state.transaction.associations, message.trid.global()))
                        {
                           if( auto instance_id = state.reserve_concurrent( service_id, common::range::make( found->second)))
                           {
                              // if the "instance" is not associated before, add it.
                              if( ! common::algorithm::contains( found->second, instance_id))
                                 found->second.push_back( instance_id);
                              
                              dispatch::lookup::reply( state, service_id, instance_id, message, pending);
                              return true;
                           }
                           return false;
                        }

                        // the gtrid is not associated before
                        if( auto instance_id = state.reserve_concurrent( service_id, {}))
                        {
                           state.transaction.associations.emplace(
                              message.trid.global(), std::vector< state::instance::concurrent::id::type>{ instance_id});

                           dispatch::lookup::reply( state, service_id, instance_id, message, pending);
                           return true;
                        }

                        // instance not found
                        return false;
                     }
                     
                  } // dispatch::lookup

                  void lookup( State& state, common::message::service::lookup::Request& message, common::chronology::duration pending = {})
                  {
                     Trace trace{ "service::manager::handle::local::service::detail::lookup"};
                     common::log::debug( "message: ", message, ", pending: ", pending);

                     using Enum = decltype( message.context.requester);

                     if( auto service_id = state.services.lookup( message.requested))
                     {
                        common::log::debug( "service_id: ", service_id);

                        switch( message.context.requester)
                        {
                           case Enum::external:
                              if( ! dispatch::lookup::internal_only( state, service_id, message, pending))
                                 dispatch::lookup::no_entry( state, message);
                              break;
                           case Enum::external_discovery:
                              if( ! dispatch::lookup::external_internal( state, service_id, message, pending))
                                 discover( state, std::move( message), state.services[ service_id].information.name); // origin name for discovery?
                              break;
                           case Enum::internal:
                              if( ! dispatch::lookup::external_internal( state, service_id, message, pending))
                                 discover( state, std::move( message), state.services[ service_id].information.name); // origin name for discovery?
                              break;
                        }
                     }
                     else if( message.context.requester == Enum::internal)
                     {
                        // we always discover, if 'requester' is internal.
                        auto name = message.requested;
                        discover( state, std::move( message), name);
                     }
                     else
                     {
                        dispatch::lookup::no_entry( state, message);
                     }
                  }

               } // detail

               auto lookup( State& state)
               {
                  return [ &state]( common::message::service::lookup::Request& message)
                  {
                     detail::lookup( state, message);
                  };
               }

               namespace discard
               {
                  auto lookup( State& state)
                  {
                     return [&state]( common::message::service::lookup::discard::Request& message)
                     {
                        Trace trace{ "service::manager::handle::service::discard::Lookup"};
                        common::log::debug( "message: ", message);

                        auto reply = common::message::reverse::type( message);

                        if( auto found = common::algorithm::find( state.pending.lookups, message.correlation))
                        {
                           common::log::debug( "found pending to discard");
                           common::log::debug( "pending: ", *found);

                           state.pending.lookups.erase( std::begin( found));
                           reply.state = decltype( reply.state)::discarded;
                        }
                        else 
                        {
                           common::log::debug( "failed to find pending to discard - check if we have reserved the service already");

                           // we need to go through all sequential instances.

                           state.instances.sequential.for_each( [ correlation = message.correlation ]( auto id, auto& ipc, auto& instance)
                           {
                              if( ! instance.idle() && instance.caller().correlation == correlation)
                              {
                                 common::log::debug( "found reserved instance: ", instance);
                                 instance.discard();
                              }
                           });

                           // regardless, we assume we've already replied.
                           reply.state = decltype( reply.state)::replied;
                        }

                        // We only send reply if caller want's it
                        if( message.reply)
                           local::optional::send( state, message.process.ipc, reply);
                     };
                  }
               } // discard

               namespace detail
               {
                  namespace handle
                  {
                     // Used by advertised above, defined here to be able to use lookup...
                     void pending( State& state, std::vector< state::service::pending::Lookup> pending)
                     {
                        Trace trace{ "service::manager::handle::local::service::detail::handle::pending"};
                        common::log::debug( "pending: ", pending);

                        if( pending.empty())
                           return;

                        auto lookup = [ &state]( auto& pending)
                        {
                           // context::wait is not relevant for pending time.
                           if( pending.request.context.semantic == decltype( pending.request.context.semantic)::wait)
                              service::detail::lookup( state, pending.request);
                           else
                              service::detail::lookup( state, pending.request, platform::time::clock::type::now() - pending.when);
                        };
                        
                        common::algorithm::for_each( pending, lookup);
                     }

                     void pending( State& state, State::update_result_t update)
                     {
                        handle::pending( state, std::move( update.pending));

                        if( update.discoverable)
                           casual::domain::discovery::discoverable::advertised( state.multiplex);
                     }

                  } // handle

                  
               } // detail
            } // service


            namespace process
            {
               namespace prepare
               {
                  namespace detail::pending
                  {
                     void lookup( State& state, std::vector< state::service::pending::Lookup> pending)
                     {
                        Trace trace{ "service::manager::handle::local::process::prepare::detail::pending::lookup"};

                        auto now = platform::time::clock::type::now();

                        for( auto& lookup : pending)
                           service::detail::lookup( state, lookup.request, now - lookup.when);
                     }
                     
                  } // detail::pending

                  auto shutdown( State& state)
                  {
                     return [ &state]( common::message::domain::process::prepare::shutdown::Request& message)
                     {
                        Trace trace{ "service::manager::handle::local::process::prepare::shutdown"};
                        common::log::debug( "message: ", message);

                        // all requested processes need to be replied some way or another. We can split them
                        // to several replies if we need to, which we do. All processes that we don't know and the ones 
                        // with no current 'reservation' we can reply directly. 
                        // 'reserved' need to be done/unreserved before we can reply them.

                        auto shutdown = state.prepare_shutdown( message.processes);
                        common::log::debug( "shutdown: ", shutdown);

                        if( shutdown.deadline)
                           local::set_timer( *shutdown.deadline);

                        // we might need to handle pending lookups for services with no instances (any more)...
                        if( ! shutdown.pending.empty())
                           detail::pending::lookup( state, std::move( shutdown.pending));
                        
                        auto is_busy = [ &state]( auto id){ return ! state.instances.sequential[ id].idle();};

                        auto [ busy, idle] = common::algorithm::partition( shutdown.instances, is_busy);
                        
                        common::log::debug( "busy: ", busy);
                        common::log::debug( "idle: ", idle);
                        common::log::debug( "unknown: ", shutdown.unknown);
                        
                        if( idle || ! shutdown.unknown.empty())
                        {
                           // we can send a reply for these directly
                           auto reply = common::message::reverse::type( message);
                           reply.processes = std::move( shutdown.unknown);

                           for( auto id : idle)
                              reply.processes.push_back( state.instances.sequential[ id].process);

                           local::optional::send( state, message.process.ipc, reply);
                        }

                        if( busy)
                        {
                           // we need to wait for 'acks' before we send a reply.

                           auto pending = common::algorithm::accumulate( busy, state.pending.shutdown.empty_pendings(), [ &state]( auto result, auto id)
                           {
                              auto& caller = state.instances.sequential[ id].caller();
                              result.emplace_back( caller.correlation, state.instances.sequential[ id].process.pid);
                              return result;
                           });

                           // we add busy instances to disabled to prevent them for doing stuff other than exit
                           common::algorithm::container::append( busy, state.disabled);

                           auto callback = [ 
                              &state,
                              message = common::message::reverse::type( message),
                              destination = message.process]
                              ( auto&& replies, auto&& outcome) mutable
                           {
                              Trace trace{ "service::manager::handle::local::process::prepare::shutdown callback"};
                              common::log::debug( "replies: ", replies);

                              // we don't need to take care of 'failed', these are taken care of by the regular process::exit

                              // take care of replies, we know that all the replies we've got comes from
                              // the reserved instances the shutdown request is all about. We correlated them from 
                              // the caller correlation id. Hence, we wouldn't be invoked in this callback if we've not
                              // received all of them (or failed)
                              message.processes = common::algorithm::transform( replies, []( auto& reply)
                              {
                                 return reply.metric.process;
                              });
   
                              common::log::debug( "message: ", message);
                              local::optional::send( state, destination.ipc, message);
                           };

                           state.pending.shutdown( std::move( pending), std::move( callback));
                           common::log::debug( "state.pending.shutdown: ", state.pending.shutdown);
                        }               
                     };
                  }

               } // prepare
            } // process

            namespace domain::discovery
            {
               namespace lookup
               {
                  auto request( State& state)
                  {
                     return [ &state]( casual::domain::message::discovery::lookup::Request& message)
                     {
                        Trace trace{ "service::manager::handle::domain::discovery::lookup::request"};
                        common::log::debug( "message: ", message);
                        
                        // check the preconditions
                        CASUAL_ASSERT( common::algorithm::is::sorted( message.content.services) && common::algorithm::is::unique( message.content.services));

                        auto predicate = [ &state, scope = message.scope]( auto service_id)
                        {
                           return state.services[ service_id].is_discoverable() && 
                              ( scope == decltype( scope)::internal ? state.services[ service_id].has_sequential() : state.services[ service_id].has_instances());
                        };

                        auto reply = common::message::reverse::type( message);

                        for( auto& name : message.content.services)
                        {
                           // filter only local/sequential services
                           if( auto service_id = state.services.lookup( name); service_id && predicate( service_id))
                           {
                              casual::domain::message::discovery::reply::content::Service result;
                              result.name = std::move( name);
                              result.category = state.services[ service_id].category;
                              result.transaction = state.services[ service_id].transaction;
                              result.visibility = state.services[ service_id].visibility.value_or( common::service::visibility::Type::discoverable);
                              result.property = state.services[ service_id].property();

                              reply.content.services.push_back( std::move( result));
                           }
                           else
                           {
                              reply.absent.services.push_back( std::move( name));
                           }
                        }

                        common::log::debug( "reply: ", reply);

                        local::optional::send( state, message.process.ipc, reply);
                     };
                  }
               } // lookup

               namespace api
               {
                  auto reply( State& state)
                  {
                     return [&state]( casual::domain::message::discovery::api::Reply& message)
                     {
                        Trace trace{ "service::manager::handle::domain::discovery::api::reply"};
                        common::log::debug( "message: ", message);

                        if( auto found = common::algorithm::find( state.pending.lookups, message.correlation))
                        {
                           if( auto service_id = state.services.lookup( found->request.requested); service_id && state.services[ service_id].instances)
                           {
                              auto pending = common::algorithm::container::extract( state.pending.lookups, std::begin( found));

                              // The requested service is now available, use
                              // the lookup to decide how to progress.
                              service::detail::lookup( state, pending.request);
                           }
                           else if( found->request.context.semantic == decltype( found->request.context.semantic)::wait)
                           {
                              // we let the lookup remain
                           }
                           else 
                           {
                              auto pending = common::algorithm::container::extract( state.pending.lookups, std::begin( found));

                              auto reply = common::message::reverse::type( pending.request);
                              reply.service.name = pending.request.requested;
                              reply.state = decltype( reply.state)::absent;

                              state.multiplex.send( pending.request.process.ipc, reply);
                           }
                        }
                     };
                  }
               } // api

               namespace fetch::known
               {
                  //! reply with all "remote" service we know of.
                  auto request( State& state)
                  {
                     return [ &state]( casual::domain::message::discovery::fetch::known::Request& message)
                     {
                        Trace trace{ "service::manager::handle::domain::discovery::fetch::known::request"};
                        common::log::debug( "message: ", message);

                        auto reply = common::message::reverse::type( message, common::process::handle());

                        // all known "remote" (not "local") services
                        state.services.for_each( [ &reply]( auto id, auto& name, auto& service)
                        {
                           if( ! service.has_sequential() && service.is_discoverable())
                              reply.content.services.push_back( name);
                        });

                        // append all waiting requests
                        for( auto& pending : state.pending.lookups)
                           if( pending.request.context.semantic == decltype( pending.request.context.semantic)::wait)
                              reply.content.services.push_back( pending.request.requested);

                        // make sure we respect the invariants
                        common::algorithm::container::sort::unique( reply.content.services);

                        common::log::debug( "reply: ", reply);
                        state.multiplex.send( message.process.ipc, reply);
                     };
                  }
               } // fetch::known

            } // domain::discovery


            //! Handles ACK from services.
            //!
            //! if there are pending request for the "acked-service" we
            //! send response directly
            auto ack( State& state)
            {
               return [ &state]( const common::message::service::call::ACK& message)
               {
                  Trace trace{ "service::manager::handle::local::ack"};
                  common::log::debug( "message: ", message);

                  // we remove possible deadline first.
                  if( auto deadline = state.pending.deadline.remove( message.correlation))
                     local::set_timer( deadline.value());
                  
                  detail::check_timeout_and_notify_TM( state, message.metric.process.pid, message.metric.trid.global());

                  // add metric event regardless
                  if( state.events.active< common::message::event::service::Calls>())
                  {
                     state.metric.add( message.metric);
                     handle::metric::batch::send( state);
                  }

                  auto instance_id = state.instances.sequential.lookup( message.metric.process.ipc);

                  if( ! instance_id)
                  {
                     common::log::debug( "failed to lookup instance for ipc: ", message.metric.process.ipc);
                     return;
                  }

                  state.unreserve( instance_id, message.metric);

                  if( auto found = common::algorithm::find( state.disabled, instance_id))
                  {
                     // we let the shutdown task take care of it.
                     state.pending.shutdown( message);
                     return;
                  }

                  auto& instance = state.instances.sequential[ instance_id];

                  common::log::debug( "instance: ", instance);

                  // Check if there are pending request for services that this
                  // instance has.

                  auto has_pending = [ &state, instance]( const auto& pending)
                  {
                     return instance.service( state.services.lookup( pending.request.requested));
                  };

                  if( auto found = common::algorithm::find_if( state.pending.lookups, has_pending))
                  {
                     common::log::debug( "found pending: ", *found);

                     auto pending = common::algorithm::container::extract( state.pending.lookups, std::begin( found));

                     // We now know that there are one idle server that has advertised the
                     // requested service (we've just marked it as idle...).
                     // We can use the normal request to get the response
                     service::detail::lookup( state, pending.request, platform::time::clock::type::now() - pending.when);
                  }
 
               };
            }


            namespace configuration
            {
               namespace update
               {

                  auto request( State& state)
                  {
                     return [&state]( casual::configuration::message::update::Request& message)
                     {
                        Trace trace{ "service::manager::handle::local::configuration::update::request"};
                        common::log::debug( "message: ", message);

                        manager::configuration::conform( state, transform::configuration( state), std::move( message.model.service));

                        auto reply = common::message::reverse::type( message);
                        state.multiplex.send( message.process.ipc, reply);
                     };
                  }
               } // update

               auto request( State& state)
               {
                  return [&state]( casual::configuration::message::Request& message)
                  {
                     Trace trace{ "service::manager::handle::local::configuration::request"};
                     common::log::debug( "message: ", message);

                     auto reply = common::message::reverse::type( message);

                     reply.model.service = transform::configuration( state);
                     state.multiplex.send( message.process.ipc, reply);
                  };
               }

            } // configuration

            namespace shutdown
            {
               auto request( State& state)
               {
                  return [&state]( const common::message::shutdown::Request& message)
                  {
                     Trace trace{ "service::manager::handle::local::shutdown::request"};
                     common::log::debug( "message: ", message);

                     state.runlevel = state::Runlevel::shutdown;
                     
                  };

               }
            } // shutdown
            
         } // <unnamed>
      } // local


      dispatch_type create( State& state)
      {
         return dispatch_type{
            common::message::dispatch::handle::defaults( state),
            common::event::listener( 
               handle::local::event::process::exit( state),
               handle::local::event::transaction::disassociate( state)
            ),
            handle::local::process::prepare::shutdown( state),
            handle::local::service::advertise( state),
            handle::local::service::lookup( state),
            handle::local::service::discard::lookup( state),
            handle::local::service::concurrent::advertise( state),
            handle::local::service::concurrent::metric( state),
            handle::local::ack( state),
            handle::local::event::subscription::begin( state),
            handle::local::event::subscription::end( state),
            handle::local::domain::discovery::lookup::request( state),
            handle::local::domain::discovery::api::reply( state),
            handle::local::domain::discovery::fetch::known::request( state),
            handle::local::configuration::update::request( state),
            handle::local::configuration::request( state),
            handle::local::shutdown::request( state),
            // will advertise the services directly to our state
            state.admin_services.initialize( admin::services( state), state),
         };
      }

   } // service::manager::handle
} // casual
