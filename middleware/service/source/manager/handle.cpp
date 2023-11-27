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

#include "common/server/lifetime.h"
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
   using namespace common;

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
            namespace optional
            {
               template< typename D, typename M>
               auto send( State& state, D&& device, M&& message)
               {
                  log::line( verbose::log, "send message: ", message);
                  return state.multiplex.send( device, message);
               }
            } // optional

            namespace error
            {
               auto reply( State& state, const state::instance::Caller& caller, common::code::xatmi code)
               {
                  Trace trace{ "service::manager::handle::local::error::reply"};
                  log::line( verbose::log, "caller: ", caller, ", code: ", code);

                  common::message::service::call::Reply message;
                  message.correlation = caller.correlation;
                  message.code.result = code; 

                  state.multiplex.send( caller.process.ipc, std::move( message));
               }
            } // error

            namespace metric
            {
               void send( State& state)
               {
                  state.events( state.multiplex, state.metric.extract());
               }
            } // metric

            namespace discovery
            {
               auto send( State& state, std::vector< std::string> services, const strong::correlation::id& correlation = strong::correlation::id::generate())
               {
                  Trace trace{ "service::manager::handle::local::discovery::send"};
                  return casual::domain::discovery::request( state.multiplex, std::move( services), {}, correlation);
               }
            }

         } // <unnamed>
      } // local

      namespace comply
      {
         void configuration( State& state, casual::configuration::Model model)
         {
            Trace trace{ "service::manager::handle::comply::configuration"};
            log::line( verbose::log, "model: ", model);

            environment::normalize( model);

            state.timeout = model.service.global.timeout;
            state.restriction = std::move( model.service.restriction);

            auto add_service = [&]( auto& service)
            {
               state::Service result;
               result.information.name = service.name;

               auto service_timeout = [ &state]( auto& timeout)
               {
                  casual::configuration::model::service::Timeout result;
                  result.duration = algorithm::coalesce( timeout.duration, state.timeout.duration).value_or( platform::time::unit::zero());
                  result.contract = algorithm::coalesce( timeout.contract, state.timeout.contract).value_or( common::service::execution::timeout::contract::Type::linger);
                  return result;
               };
               result.timeout = service_timeout( service.timeout);
               result.visibility = service.visibility;

               if( service.routes.empty())
                  state.services.emplace( std::move( service.name), std::move( result));
               else 
               {
                  for( auto& route : service.routes)
                  {
                     state.services.emplace( route, result);
                     state.reverse_routes.emplace( route, service.name);
                  }

                  state.routes.emplace( service.name, std::move( service.routes));
               }
            };

            algorithm::for_each( model.service.services, add_service);

            log::line( verbose::log, "state: ", state);

         }
      } // comply

      void timeout( State& state)
      {
         Trace trace{ "service::manager::handle::timeout"};

         const auto now = platform::time::clock::type::now();

         auto expired = state.pending.deadline.expired( now);
         log::line( verbose::log, "expired: ", expired);

         auto send_error_reply = [&state]( auto& entry)
         {
            if( auto caller = entry.service->consume( entry.correlation))
            {
               // keep track of instance until we get an ACK, or the server dies
               // We need to notify TM if this call was in transaction.
               state.timeout_instances.push_back( entry.target);

               local::error::reply( state, caller, common::code::xatmi::timeout);

               // send event, at least domain-manager want's to know...
               common::message::event::process::Assassination event{ common::process::handle()};
               event.target = entry.target;
               event.contract = entry.service->timeout.contract.value_or( common::service::execution::timeout::contract::Type::linger);
               common::event::send( event);
            }
         };

         algorithm::for_each( expired.entries, send_error_reply);

         if( expired.deadline)
            signal::timer::set( expired.deadline.value() - now);
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
            communication::ipc::inbound::device().push( std::move( event));
         }
      } // process

      namespace local
      {
         namespace
         {
            namespace service::detail::handle
            {
               // defined after lookup, below
               void pending( State& state, std::vector< state::service::pending::Lookup>&& pending);
                  
            } // service::detail::handle

            namespace event
            { 
               namespace process
               {
                  auto exit( State& state)
                  {
                     return [&state]( common::message::event::process::Exit& event)
                     {
                        Trace trace{ "service::manager::handle::local::event::process::detail::exit"};
                        log::line( verbose::log, "event: ", event);

                        // we need to check if the dead process has anyone waiting for a reply
                        if( auto found = common::algorithm::find( state.instances.sequential, event.state.pid))
                        {
                           // if the callee (instance) is busy and the caller is not 'consumed' (due to a timeout and a 'timeout' reply has
                           // already been sent) we send error reply
                           if( found->second.state() == decltype( found->second.state())::busy && found->second.caller())
                           {
                              auto& instance = found->second;
                              log::line( common::log::category::error, code::casual::invalid_semantics, 
                                 " callee terminated with pending reply to caller - callee: ", 
                                 event.state.pid, " - caller: ", instance.caller().process.pid);

                              log::line( common::log::category::verbose::error, "instance: ", instance);

                              local::error::reply( state, instance.caller(), common::code::xatmi::service_error);
                           }
                        }

                        state.pending.shutdown.failed( event.state.pid);
                        state.remove( event.state.pid);

                        // It might be an assassinated instance.
                        // TODO: this is a glitch - the dead instance could have been involved with
                        // a resource after the timeout (reply to caller -> rollback) and before it's
                        // death.
                        algorithm::container::erase( state.timeout_instances, event.state.pid);

                        // The dead process might be the last instance that supplied a given set of services. 
                        // We need to check pending lookups... We just "invalidate and re-lookup"
                        service::detail::handle::pending( state, std::exchange( state.pending.lookups, {}));
                     };
                  }
               } // process

               namespace subscription
               {
                  auto begin( State& state)
                  {
                     return [&state]( common::message::event::subscription::Begin& message)
                     {
                        Trace trace{ "service::manager::handle::event::subscription::Begin"};
                        log::line( verbose::log, "message: ", message);

                        state.events.subscription( message);
                     };
                  }

                  auto end( State& state)
                  {
                     return [&state]( common::message::event::subscription::End& message)
                     {
                        Trace trace{ "service::manager::handle::event::subscription::End"};
                        log::line( verbose::log, "message: ", message);

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
                     log::line( verbose::log, "message: ", message);

                     // some pending might got resolved, from the update
                     detail::handle::pending( state, state.update( message));
                  };
               }

               namespace concurrent
               {
                  auto advertise( State& state)
                  {
                     return [&state]( common::message::service::concurrent::Advertise& message)
                     {
                        Trace trace{ "service::manager::handle::service::concurrent::advertise"};
                        common::log::line( verbose::log, "message: ", message);

                        // some pending might got resolved.
                        detail::handle::pending( state, state.update( message));
                     };
                  }

                  auto metric( State& state)
                  {
                     return [&state]( common::message::event::service::Calls& message)
                     {
                        Trace trace{ "service::manager::handle::service::concurrent::metric"};
                        log::line( verbose::log, "message: ", message);

                        for( auto& metric : message.metrics)
                           if( auto service = state.service( metric.service))
                              service->metric.update( metric);

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
                  void discover( State& state, common::message::service::lookup::Request&& message, const std::string& name)
                  {
                     Trace trace{ "service::manager::handle::local::service::detail::discover"};

                     log::line( verbose::log, "failed to find service: ", name, " - action: discover");

                     auto send_reply = execute::scope( [&]()
                     {
                        log::line( log, "no instances found for service: ", name);

                        // Server that hosts the requested service is not found.
                        // We propagate this by having absent state
                        auto reply = common::message::reverse::type( message);
                        reply.service.name = message.requested;
                        reply.state = decltype( reply.state)::absent;

                        local::optional::send( state, message.process.ipc, reply);
                     });

                     {
                        log::line( log, "no instances found for service: ", name, " - action: ask neighbor domains");

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
                        log::line( verbose::log, "failed to find service: ", message.requested, " - action: reply with ", code::xatmi::no_entry);
                        auto reply = common::message::reverse::type( message);
                        reply.service.name = message.requested;
                        reply.state = decltype( reply.state)::absent;

                        state.multiplex.send( message.process.ipc, reply);
                     }

                     void reply( State& state, state::Service& service, const common::process::Handle& destination, common::message::service::lookup::Request& message, platform::time::unit pending)
                     {
                        log::line( verbose::log, "'reserved' instance: ", destination);

                        auto reply = common::message::reverse::type( message);
                        reply.service = service.information;
                        reply.service.timeout.duration = service.timeout.duration.value_or( platform::time::unit::zero());
                        reply.state = decltype( reply.state)::idle;
                        reply.process = destination;
                        reply.pending = pending;

                        if( service.information.name != message.requested)
                           reply.service.requested = message.requested;

                        if( service.timeoutable())
                        {
                           auto now = platform::time::clock::type::now();

                           auto next = state.pending.deadline.add( {
                              now + reply.service.timeout.duration,
                              message.correlation,
                              destination.pid,
                              &service});

                           if( next)
                              signal::timer::set( next.value() - now);
                        }
                        else if( message.deadline)
                        {
                           reply.service.timeout.duration = message.deadline.value() - platform::time::clock::type::now();
                        }                             

                        // send reply, if caller gone, we discard the reservation.
                        state.multiplex.send( message.process.ipc, reply, [ &state, pid = destination.pid]( auto& destination, auto& complete)
                        {
                           if( auto found = algorithm::find( state.instances.sequential, pid))
                              found->second.discard();
                        });
                     }

                     void pending( State& state, const state::Service& service, common::message::service::lookup::Request& message)
                     {
                        auto reply = common::message::reverse::type( message);
                        reply.service = service.information;

                        switch( message.context.semantic)
                        {
                           using Semantic = decltype( message.context.semantic);

                           case Semantic::no_reply:
                           {
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
                           case Semantic::no_busy_intermediate:
                           {
                              // the caller does not want to get a busy intermediate, only want's to wait until
                              // the service is idle. That is, we don't need to send timeout and
                              // stuff to the caller.
                              //
                              // If this is from another domain the actual timeouts and stuff can differ
                              // and this might be a problem?
                              // TODO semantics: something we need to address? probably not,
                              // since we can't make it 100% any way...)
                              state.pending.lookups.emplace_back( std::move( message), platform::time::clock::type::now());

                              break;
                           }
                           case Semantic::regular:
                           {
                              // send busy-message to caller, to set timeouts and stuff
                              reply.state = decltype( reply.state)::busy;

                              if( local::optional::send( state, message.process.ipc, reply))
                              {
                                 // All instances are busy, we stack the request
                                 state.pending.lookups.emplace_back( std::move( message), platform::time::clock::type::now());
                              }

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

                     bool internal_only( State& state, state::Service& service, common::message::service::lookup::Request& message, platform::time::unit pending)
                     {  
                        if( service.is_sequential())
                        {
                           auto get_caller = []( const auto& message) -> common::process::Handle
                           {
                              if( message.no_reply())
                                 return {};
                              return message.process;
                           };

                           if( auto destination = service.reserve_sequential( get_caller( message), message.correlation))
                              dispatch::lookup::reply( state, service, destination, message, pending);
                           else
                              dispatch::lookup::pending( state, service, message);
                           return true;
                        }
                        return false;
                     }

                     bool external_internal( State& state, state::Service& service, common::message::service::lookup::Request& message, platform::time::unit pending)
                     {
                        if( dispatch::lookup::internal_only( state, service, message, pending))
                           return true;

                        if( auto destination = service.reserve_concurrent( message.process, message.correlation))
                        {
                           dispatch::lookup::reply( state, service, destination, message, pending);
                           return true;
                        }

                        return false;
                     }
                     
                  } // dispatch::lookup

                  void lookup( State& state, common::message::service::lookup::Request& message, platform::time::unit pending)
                  {
                     Trace trace{ "service::manager::handle::local::service::detail::lookup"};
                     log::line( verbose::log, "message: ", message, ", pending: ", pending);

                     using Enum = decltype( message.context.requester);

                     if( auto service = state.service( message.requested))
                     {
                        log::line( verbose::log, "service: ", service);

                        switch( message.context.requester)
                        {
                           case Enum::external:
                              if( ! dispatch::lookup::internal_only( state, *service, message, pending))
                                 dispatch::lookup::no_entry( state, message);
                              break;
                           case Enum::external_discovery:
                              if( ! dispatch::lookup::external_internal( state, *service, message, pending))
                                 discover( state, std::move( message), service->information.name);
                              break;
                           case Enum::internal:
                              if( ! dispatch::lookup::external_internal( state, *service, message, pending))
                                 discover( state, std::move( message), service->information.name);
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
                  return [&state]( common::message::service::lookup::Request& message)
                  {
                     detail::lookup( state, message, platform::time::unit{});
                  };
               }

               namespace discard
               {
                  auto lookup( State& state)
                  {
                     return [&state]( common::message::service::lookup::discard::Request& message)
                     {
                        Trace trace{ "service::manager::handle::service::discard::Lookup"};
                        log::line( verbose::log, "message: ", message);

                        auto reply = message::reverse::type( message);

                        if( auto found = algorithm::find( state.pending.lookups, message.correlation))
                        {
                           log::line( log, "found pending to discard");
                           log::line( verbose::log, "pending: ", *found);

                           state.pending.lookups.erase( std::begin( found));
                           reply.state = decltype( reply.state)::discarded;
                        }
                        else 
                        {
                           log::line( log, "failed to find pending to discard - check if we have reserved the service already");

                           // we need to go through all instances.

                           auto reserved_service = [&message]( auto& tuple)
                           {
                              auto& instance = tuple.second;
                              return ! instance.idle() && instance.caller().correlation == message.correlation;
                           };

                           if( auto found = algorithm::find_if( state.instances.sequential, reserved_service))
                           {
                              auto& instance = found->second;
                              log::line( log, "found reserved instance");
                              log::line( verbose::log, "instance: ", instance);

                              instance.discard();
                           }

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
                     void pending( State& state, std::vector< state::service::pending::Lookup>&& pending)
                     {
                        Trace trace{ "service::manager::handle::local::service::detail::handle::pending"};
                        log::line( verbose::log, "pending: ", pending);

                        if( pending.empty())
                           return;

                        auto lookup = [ &state, now = platform::time::clock::type::now()]( auto& pending)
                        {
                           // context::wait is not relevant for pending time.
                           if( pending.request.context.semantic == decltype( pending.request.context.semantic)::wait)
                              service::detail::lookup( state, pending.request, {});
                           else
                              service::detail::lookup( state, pending.request, platform::time::clock::type::now() - pending.when);
                        };
                        
                        algorithm::for_each( pending, lookup);
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
                     void lookup( State& state, std::vector< std::string> origin_services)
                     {
                        Trace trace{ "service::manager::handle::local::process::prepare::detail::pending::lookup"};

                        // normalize origin-services, to take routes in to account...
                        auto services = algorithm::accumulate( std::move( origin_services), std::vector< std::string>{}, [&state]( auto result, auto& name)
                        {
                           if( auto found = algorithm::find( state.routes, name))
                              algorithm::container::append( found->second, result);
                           else
                              result.push_back( std::move( name));

                           return result;
                        });

                        algorithm::container::sort::unique( services);

                        // extract the corresponding pending lookups and 'emulate' new lookups, if any.
                        {
                           auto lookups = algorithm::container::extract( 
                              state.pending.lookups, 
                              std::get< 0>( algorithm::intersection( state.pending.lookups, services)));

                           const auto now = platform::time::clock::type::now();

                           for( auto& lookup : lookups)
                              service::detail::lookup( state, lookup.request, now - lookup.when);
                        }
                     }
                     
                  } // detail::pending

                  auto shutdown( State& state)
                  {
                     return [&state]( common::message::domain::process::prepare::shutdown::Request& message)
                     {
                        Trace trace{ "service::manager::handle::local::process::prepare::shutdown"};
                        log::line( verbose::log, "message: ", message);

                        // all requested processes need to be replied some way or another. We can split them
                        // to several replies if we need to, which we do. All processes that we don't know and the ones 
                        // with no current 'reservation' we can reply directly. 
                        // 'reserved' need to be done/unreserved before we can reply them.

                        auto [ services, instances, unknown] = state.prepare_shutdown( message.processes);
                        
                        log::line( verbose::log, "services: ", services);

                        // we might need to handle pending lookups for services with no instances (any more)...
                        if( ! services.empty())
                           detail::pending::lookup( state, services);

                        auto [ busy, idle] = algorithm::partition( instances, []( auto& instance){ return ! instance.idle();});
                        
                        log::line( verbose::log, "busy: ", busy);
                        log::line( verbose::log, "idle: ", idle);
                        log::line( verbose::log, "unknown: ", unknown);
                        
                        if( idle || ! unknown.empty())
                        {
                           // we can send a reply for these directly
                           auto reply = common::message::reverse::type( message);
                           reply.processes = std::move( unknown);

                           algorithm::transform( idle, std::back_inserter( reply.processes), []( auto& instance)
                           {
                              return instance.process;
                           });
                           local::optional::send( state, message.process.ipc, reply);
                        }

                        if( busy)
                        {
                           // we need to wait for 'acks' before we send a reply.

                           auto pending = algorithm::accumulate( busy, state.pending.shutdown.empty_pendings(), []( auto result, auto& instance)
                           {
                              auto& caller = instance.caller();
                              result.emplace_back( caller.correlation, instance.process.pid);
                              return result;
                           });

                           auto callback = [ 
                              &state,
                              message = common::message::reverse::type( message), 
                              instances = algorithm::container::vector::create( busy),
                              destination = message.process]
                              ( auto&& replies, auto&& outcome) mutable
                           {
                              Trace trace{ "service::manager::handle::local::process::prepare::shutdown callback"};
                              log::line( verbose::log, "replies: ", replies);

                              auto failed = algorithm::filter( outcome, []( auto& pending){ return pending.state == decltype( pending.state)::failed;});
                              log::line( verbose::log, "failed: ", failed);

                              // take care of failed, if any
                              for( auto& failed : failed)
                                 if( auto found = algorithm::find( instances, failed.id))
                                    local::error::reply( state, found->caller(), code::xatmi::service_error);

                              // take care of replies
                              algorithm::for_each( replies, [&]( auto& reply)
                              {
                                 if( auto found = algorithm::find( instances, reply.metric.process.pid))
                                 {
                                    found->unreserve( reply.metric);
                                    message.processes.push_back( reply.metric.process);
                                    log::line( verbose::log, "found: ", *found);
                                 }
                              }); 
                              
                              local::optional::send( state, destination.ipc, message);

                           };

                           state.pending.shutdown( std::move( pending), std::move( callback));
                           log::line( verbose::log, "state.pending.shutdown: ", state.pending.shutdown);
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
                        common::log::line( verbose::log, "message: ", message);
                        
                        // check the preconditions
                        CASUAL_ASSERT( algorithm::is::sorted( message.content.services) && algorithm::is::unique( message.content.services));

                        auto predicate = [ scope = message.scope]( auto* service)
                        {
                           return service && service->is_discoverable() && 
                              ( scope == decltype( scope)::internal ? service->is_sequential() : service->has_instances());
                        };

                        auto reply = common::message::reverse::type( message);

                        for( auto& name : message.content.services)
                        {
                           if( auto service = state.service( name); predicate( service))
                           {
                              reply.content.services.emplace_back( std::move( name),
                                 service->information.category,
                                 service->information.transaction, 
                                 service->information.visibility, 
                                 service->property());
                           }
                           else
                           {
                              reply.absent.services.push_back( std::move( name));
                           }
                        }

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
                        common::log::line( verbose::log, "message: ", message);

                        if( auto found = algorithm::find( state.pending.lookups, message.correlation))
                        {
                           auto pending = algorithm::container::extract( state.pending.lookups, std::begin( found));

                           if( auto service = state.service( pending.request.requested); service && service->instances)
                           {
                              // The requested service is now available, use
                              // the lookup to decide how to progress.
                              service::lookup( state)( pending.request);
                           }
                           else if( pending.request.context.semantic == decltype( pending.request.context.semantic)::wait)
                           {
                              // we put the pending back.
                              state.pending.lookups.push_back( std::move( pending));
                           }
                           else 
                           {
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
                        log::line( verbose::log, "message: ", message);

                        auto reply = common::message::reverse::type( message, common::process::handle());

                        // all known "remote" (not "local") services
                        reply.content.services = algorithm::accumulate( state.services, std::move( reply.content.services), []( auto result, auto& pair)
                        {
                           const auto& [ name, service] = pair;
                           if( ! service.is_sequential())
                              result.push_back( name);

                           return result;
                        });

                        // append all waiting requests
                        for( auto& pending : state.pending.lookups)
                           if( pending.request.context.semantic == decltype( pending.request.context.semantic)::wait)
                              reply.content.services.push_back( pending.request.requested);

                        // make sure we respect the invariants
                        algorithm::container::sort::unique( reply.content.services);

                        log::line( verbose::log, "reply: ", reply);
                        state.multiplex.send( message.process.ipc, reply);
                     };
                  }
               } // fetch::known

            } // domain::discovery

            namespace detail
            {
               void check_timeout_and_notify_TM( State& state, const common::message::service::call::ACK& ack)
               {
                  log::line( verbose::log, "state.timeout_instances: ", state.timeout_instances);

                  if( ! ack.metric.trid)
                     return;
                  
                  if( auto found = algorithm::find( state.timeout_instances, ack.metric.process.pid))
                  {
                     state.timeout_instances.erase( std::begin( found));

                     // Rollback the transaction (again). If TM got instances involved a rollback will be executed, 
                     // otherwise it will be a "no-op".
                     // TODO: we might want to have a dedicated message for this. SM should not order a rollback?
                     message::transaction::rollback::Request message{ common::process::handle()};
                     message.trid = ack.metric.trid;
                     optional::send( state, communication::instance::outbound::transaction::manager::device(), message);
                  }
               }
               
            } // detail


            //! Handles ACK from services.
            //!
            //! if there are pending request for the "acked-service" we
            //! send response directly
            auto ack( State& state)
            {
               return [ &state]( const common::message::service::call::ACK& message)
               {
                  Trace trace{ "service::manager::handle::local::ack"};
                  log::line( verbose::log, "message: ", message);


                  // we remove possible deadline first.
                  if( auto deadline = state.pending.deadline.remove( message.correlation))
                     signal::timer::set( deadline.value());

                  
                  detail::check_timeout_and_notify_TM( state, message);

                  // add metric event regardless
                  if( state.events.active< common::message::event::service::Calls>())
                  {
                     state.metric.add( message.metric);
                     handle::metric::batch::send( state);
                  }

                  // This message can only come from a local instance
                  if( auto instance = state.sequential( message.metric.process.pid))
                  {
                     log::line( verbose::log, "instance: ", *instance);

                     instance->unreserve( message.metric);
                     

                     // Check if there are pending request for services that this
                     // instance has.

                     auto has_pending = [&state, instance]( const auto& pending)
                     {
                        if( instance->service( pending.request.requested))
                           return true;
                        
                        // we need to check routes...
                        if( auto found = algorithm::find( state.reverse_routes, pending.request.requested))
                           return instance->service( found->second);

                        return false;
                     };

                     if( auto found = common::algorithm::find_if( state.pending.lookups, has_pending))
                     {
                        log::line( verbose::log, "found pending: ", *found);

                        auto pending = algorithm::container::extract( state.pending.lookups, std::begin( found));

                        // We now know that there are one idle server that has advertised the
                        // requested service (we've just marked it as idle...).
                        // We can use the normal request to get the response
                        service::detail::lookup( state, pending.request, platform::time::clock::type::now() - pending.when);
                     }
                  }
                  else
                  {
                     // assume pending shutdown
                     state.pending.shutdown( message);
                  }
               };
            }

            namespace timeout::rollback
            {
               auto reply( State& state)
               {
                  return []( const message::transaction::rollback::Reply& message)
                  {
                     Trace trace{ "service::manager::handle::local::timeout::rollback::reply"};
                     log::line( verbose::log, "message: ", message);
                  
                     if( message.state != decltype( message.state)::ok)
                     {
                        log::line( log::category::error, message.state, " timeout rollback failed for trid: ", message.trid);
                        log::line( log::category::verbose::error, "message: ", message);
                     }
                  };
               }
               
            } // timeout::rollback 


            namespace configuration
            {
               namespace update
               {

                  auto request( State& state)
                  {
                     return [&state]( casual::configuration::message::update::Request& message)
                     {
                        Trace trace{ "service::manager::handle::local::configuration::update::request"};
                        log::line( verbose::log, "message: ", message);

                        manager::configuration::conform( state, transform::configuration( state), std::move( message.model.service));

                        auto reply = message::reverse::type( message);
                        state.multiplex.send( message.process.ipc, reply);
                     };
                  }
               } // update

               auto request( State& state)
               {
                  return [&state]( casual::configuration::message::Request& message)
                  {
                     Trace trace{ "service::manager::handle::local::configuration::request"};
                     log::line( verbose::log, "message: ", message);

                     auto reply = message::reverse::type( message);

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
                     log::line( verbose::log, "message: ", message);

                     state.runlevel = state::Runlevel::shutdown;
                     
                  };

               }
            } // shutdown


            //! service-manager needs to have it's own policy for callee::handle::basic_call, since
            //! we can't communicate with blocking to the same queue (with read, who is
            //! going to write? with write, what if the queue is full?)
            struct Policy
            {

               Policy( manager::State& state) : m_state( &state) {}

               Policy( Policy&&) = default;
               Policy& operator = ( Policy&&) = default;

               void configure( common::server::Arguments&& arguments)
               {
                  m_state->connect_manager( std::move( arguments.services));
               }

               void reply( common::strong::ipc::id id, common::message::service::call::Reply& message)
               {
                  communication::device::blocking::send( id, message);
               }

               void ack( const common::message::service::call::ACK& ack)
               {
                  local::ack( *m_state)( ack);
               }
               
               template< typename... Ts>
               void transaction( Ts&&...)
               {
                  // service-manager doesn't bother with transactions...
               }

               common::message::service::Transaction transaction( bool commit)
               {
                  // service-manager doesn't bother with transactions...
                  return {};
               }

               void forward( common::service::invoke::Forward&& forward, const common::message::service::call::callee::Request& message)
               {
                  assert( ! "can't forward within service-manager");
                  std::terminate();
               }

               void statistics( common::strong::ipc::id, common::message::event::service::Call&)
               {
                  // We don't collect statistics for the service-manager
               }

            private:
               manager::State* m_state;
            };

            using Call = common::server::handle::basic_call< Policy>;
            
         } // <unnamed>
      } // local


      dispatch_type create( State& state)
      {
         return dispatch_type{
            common::message::dispatch::handle::defaults( state),
            common::event::listener( 
               handle::local::event::process::exit( state)
            ),
            handle::local::process::prepare::shutdown( state),
            handle::local::service::advertise( state),
            handle::local::service::lookup( state),
            handle::local::service::discard::lookup( state),
            handle::local::service::concurrent::advertise( state),
            handle::local::service::concurrent::metric( state),
            handle::local::ack( state),
            handle::local::timeout::rollback::reply( state),
            handle::local::event::subscription::begin( state),
            handle::local::event::subscription::end( state),
            handle::local::Call{ admin::services( state), state},
            handle::local::domain::discovery::lookup::request( state),
            handle::local::domain::discovery::api::reply( state),
            handle::local::domain::discovery::fetch::known::request( state),
            handle::local::configuration::update::request( state),
            handle::local::configuration::request( state),
            handle::local::shutdown::request( state),
         };
      }

   } // service::manager::handle
} // casual
