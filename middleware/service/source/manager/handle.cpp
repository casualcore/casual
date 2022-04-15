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
#include "common/process.h"
#include "common/message/dispatch.h"
#include "common/message/handle.h"
#include "common/message/internal.h"
#include "common/event/listen.h"
#include "common/event/send.h"

#include "common/communication/instance.h"

#include "configuration/message.h"
#include "configuration/model/change.h"

#include "domain/pending/message/send.h"
#include "domain/discovery/api.h"

// std
#include <vector>
#include <string>

namespace casual
{
   using namespace common;

   namespace service::manager
   {
      namespace ipc
      {
         common::communication::ipc::inbound::Device& device()
         {
            return common::communication::ipc::inbound::device();
         }
      } // ipc

      namespace handle
      {
         namespace local
         {
            namespace
            {
               namespace optional
               {
                  template< typename D, typename M>
                  auto send( D&& device, M&& message)
                  {
                     log::line( verbose::log, "send message: ", message);
                     return communication::device::blocking::optional::send( device, message);
                  }
               } // optional

               namespace eventually
               {
                  template<  typename M>
                  bool send( const common::process::Handle& destination, M&& message)
                  {
                     Trace trace{ "service::manager::handle::local::eventually::send"};
                     log::line( verbose::log, "message: ", message);

                     try
                     {
                        if( ! communication::device::non::blocking::send( destination.ipc, message))
                           casual::domain::pending::message::send( destination, std::forward< M>( message));

                        return true;
                     }
                     catch( ...)
                     {
                        auto error = exception::capture();
                        if( error.code() != code::casual::communication_unavailable)
                           throw;
                           
                        log::line( log, error, " destination unavailable - ", destination);
                        return false;
                     }

                  }
               } // eventually

               namespace error
               {
                  auto reply( const state::instance::Caller& caller, common::code::xatmi code)
                  {
                     Trace trace{ "service::manager::handle::local::error::reply"};
                     log::line( verbose::log, "caller: ", caller, ", code: ", code);

                     common::message::service::call::Reply message;
                     message.correlation = caller.correlation;
                     message.code.result = code; 

                     handle::local::eventually::send( caller.process, std::move( message));
                  }
               } // error

               namespace metric
               {
                  void send( State& state)
                  {
                     auto pending = state.events( state.metric.message());
                     state.metric.clear();

                     if( ! common::message::pending::non::blocking::send( pending))
                        casual::domain::pending::message::send( pending);
                  }
               } // metric

               namespace discovery
               {
                  auto send( std::vector< std::string> services, const strong::correlation::id& correlation = strong::correlation::id::generate())
                  {
                     Trace trace{ "service::manager::handle::local::discovery::send"};
                     return casual::domain::discovery::request( std::move( services), {}, correlation);
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

               state.timeout = model.service.timeout;
               state.restrictions = model.service.restrictions;

               auto add_service = [&]( auto& service)
               {
                  state::Service result;
                  result.information.name = service.name;
                  result.timeout = service.timeout;

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

            auto send_error_reply = []( auto& entry)
            {
               if( auto caller = entry.service->consume( entry.correlation))
               {
                  local::error::reply( caller, common::code::xatmi::timeout);

                  // send event, at least domain-manager want's to know...
                  common::message::event::process::Assassination event{ common::process::handle()};
                  event.target = entry.target;
                  event.contract = entry.service->timeout.contract;
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

                           // we need to check if the dead process has anyone wating for a reply
                           if( auto found = common::algorithm::find( state.instances.sequential, event.state.pid))
                           {
                              // if the callee (instance) is busy and the caller is not 'consumed' (due to a timeout and a 'timeout' reply has
                              // allready been sent) we send error reply
                              if( found->second.state() == decltype( found->second.state())::busy && found->second.caller())
                              {
                                 auto& instance = found->second;
                                 log::line( common::log::category::error, code::casual::invalid_semantics, 
                                    " callee terminated with pending reply to caller - callee: ", 
                                    event.state.pid, " - caller: ", instance.caller().process.pid);

                                 log::line( common::log::category::verbose::error, "instance: ", instance);

                                 local::error::reply( instance.caller(), common::code::xatmi::service_error);
                              }
                           }

                           state.pending.shutdown.failed( event.state.pid);
                           state.remove( event.state.pid);
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
                  namespace detail
                  {
                     namespace handle
                     {
                        // defined after lookup, below
                        void pending( State& state, std::vector< state::service::pending::Lookup>&& pending);
                     } // handle

                     
                  } // detail

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

                        auto send_reply = execute::scope( [&]()
                        {
                           log::line( log, "no instances found for service: ", name);

                           // Server that hosts the requested service is not found.
                           // We propagate this by having absent state
                           auto reply = common::message::reverse::type( message);
                           reply.service.name = message.requested;
                           reply.state = decltype( reply.state)::absent;

                           local::optional::send( message.process.ipc, reply);
                        });

                        {
                           log::line( log, "no instances found for service: ", name, " - action: ask neighbor domains");

                           if( local::discovery::send( { name}, message.correlation) || message.context == decltype( message.context)::wait)
                           {
                              // we sent the request OR the caller is willing to wait for future 
                              // advertised services

                              state.pending.lookups.emplace_back( std::move( message), platform::time::clock::type::now());
                              send_reply.release();
                           }
                        }
                     }

                     void lookup( State& state, common::message::service::lookup::Request& message, platform::time::unit pending)
                     {
                        Trace trace{ "service::manager::handle::local::service::detail::lookup"};
                        log::line( verbose::log, "message: ", message, ", pending: ", pending);

                        if( auto service = state.service( message.requested))
                        {
                           if( auto handle = service->reserve( message.process, message.correlation))
                           {
                              auto reply = common::message::reverse::type( message);
                              reply.service = service->information;
                              reply.state = decltype( reply.state)::idle;
                              reply.process = handle;
                              reply.pending = pending;

                              if( service->information.name != message.requested)
                                 reply.service.requested = message.requested;

                              // note: concurrent instances are not subject to timeout within the domain
                              if( service->timeoutable())
                              {
                                 auto now = platform::time::clock::type::now();

                                 reply.service.timeout.duration = service->timeout.duration.value();

                                 auto next = state.pending.deadline.add( {
                                    now + reply.service.timeout.duration,
                                    message.correlation,
                                    handle.pid,
                                    service});

                                 if( next)
                                    signal::timer::set( next.value() - now);
                              }
                              else if( message.deadline)
                              {
                                 reply.service.timeout.duration = message.deadline.value() - platform::time::clock::type::now();
                              }                             

                              // send reply, if caller gone, we discard the reservation.
                              if( ! local::optional::send( message.process.ipc, reply))
                                 if( auto found = algorithm::find( state.instances.sequential, handle.pid))
                                    found->second.discard();

                           }
                           else if( service->instances.empty())
                           {
                              // note: vi discover on the "real service name", in case it's a route
                              discover( state, std::move( message), service->information.name);
                           }
                           else
                           {
                              auto reply = common::message::reverse::type( message);
                              reply.service = service->information;

                              switch( message.context)
                              {
                                 using Context = decltype( message.context);

                                 case Context::forward:
                                 {
                                    // The intention is "send and forget", or a plain forward, we use our forward-cache for this
                                    reply.process = state.forward;

                                    // Caller will think that service is idle, that's the whole point
                                    // with our forward.
                                    reply.state = decltype( reply.state)::idle;

                                    local::optional::send( message.process.ipc, reply);

                                    break;
                                 }
                                 case Context::no_busy_intermediate:
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
                                 case Context::regular:
                                 {
                                    // send busy-message to caller, to set timeouts and stuff
                                    reply.state = decltype( reply.state)::busy;

                                    if( local::optional::send( message.process.ipc, reply))
                                    {
                                       // All instances are busy, we stack the request
                                       state.pending.lookups.emplace_back( std::move( message), platform::time::clock::type::now());
                                    }

                                    break;
                                 }
                                 case Context::wait:
                                 {
                                    // we know the service exists, and it got instances, we do a regular pending.
                                    state.pending.lookups.emplace_back( std::move( message), platform::time::clock::type::now());
                                 }
                              }
                           }
                        }
                        else
                        {
                           auto name = message.requested;
                           discover( state, std::move( message), name);
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

                                 reply.state = decltype( reply.state)::replied;
                              }
                           }

                           // We only send reply if caller want's it
                           if( message.reply)
                              local::optional::send( message.process.ipc, reply);
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

                           auto lookup = [&state, now = platform::time::clock::type::now()]( auto& pending)
                           {
                              // context::wait is not relevant for pending time.
                              if( pending.request.context == decltype( pending.request.context)::wait)
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
                                 algorithm::append( found->second, result);
                              else
                                 result.push_back( std::move( name));

                              return result;
                           });

                           algorithm::container::trim( services, algorithm::unique( algorithm::sort( services)));

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

                           // all requested processess need to be replied some way or another. We can split them
                           // to several replies if we need to, which we do. All processses that we don't know and the ones 
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
                              auto reply = common::message::reverse::type( message, common::process::handle());
                              reply.processes = std::move( unknown);

                              algorithm::transform( idle, std::back_inserter( reply.processes), []( auto& instance)
                              {
                                 return instance.process;
                              });
                              local::optional::send( message.process.ipc, reply);
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
                                 message = common::message::reverse::type( message, common::process::handle()), 
                                 instances = range::to_vector( busy),
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
                                       local::error::reply( found->caller(), code::xatmi::service_error);

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
                                 
                                 local::optional::send( destination.ipc, message);

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
                  auto request( State& state)
                  {
                     return [&state]( casual::domain::message::discovery::Request& message)
                     {
                        Trace trace{ "service::manager::handle::domain::discovery::request"};
                        common::log::line( verbose::log, "message: ", message);

                        auto reply = common::message::reverse::type( message, common::process::handle());
                        reply.domain = common::domain::identity();
                        
                        reply.content.services = algorithm::accumulate( message.content.services, decltype( reply.content.services){}, [&state]( auto result, auto& name)
                        {
                           auto service = state.service( name);

                           // * We only answer with sequential (local) services
                           // * We don't allow other domains to access or know about our admin services.
                           if( service && ! service->instances.sequential.empty() && service->information.category != common::service::category::admin)
                           {
                              result.emplace_back(
                                 name,
                                 service->information.category,
                                 service->information.transaction);
                           }

                           return result;
                        });

                        local::optional::send( message.process.ipc, reply);
                     };
                  }

                  auto reply( State& state)
                  {
                     return [&state]( casual::domain::message::discovery::api::Reply& message)
                     {
                        Trace trace{ "service::manager::handle::domain::discovery::external::reply"};
                        common::log::line( verbose::log, "message: ", message);

                        if( auto found = algorithm::find( state.pending.lookups, message.correlation))
                        {
                           auto pending = algorithm::container::extract( state.pending.lookups, std::begin( found));

                           auto service = state.service( pending.request.requested);

                           if( service && ! service->instances.empty())
                           {
                              // The requested service is now available, use
                              // the lookup to decide how to progress.
                              service::lookup( state)( pending.request);
                           }
                           else if( pending.request.context == decltype( pending.request.context)::wait)
                           {
                              // we put the pending back. In front to be as fair as possible
                              state.pending.lookups.push_front( std::move( pending));
                           }
                           else 
                           {
                              auto reply = common::message::reverse::type( pending.request);
                              reply.service.name = pending.request.requested;
                              reply.state = decltype( reply.state)::absent;

                              communication::device::blocking::send( pending.request.process.ipc, reply);
                           }
                        }
                     };
                  }

                  namespace external::needs
                  {
                     //! reply with all known external and what we're waiting for.
                     auto request( const State& state)
                     {
                        return [&state]( const casual::domain::message::discovery::needs::Request& message)
                        {
                           Trace trace{ "service::manager::handle::domain::discovery::external::needs::request"};
                           log::line( verbose::log, "message: ", message);

                           auto reply = common::message::reverse::type( message, common::process::handle());

                           // all pending
                           reply.content.services = algorithm::transform( state.pending.lookups, []( auto& value)
                           {
                              return value.request.requested;
                           });

                           // all "external" we know of.
                           reply.content.services = algorithm::accumulate( state.services, std::move( reply.content.services), []( auto result, auto& value)
                           {
                              if( ! value.second.instances.concurrent.empty())
                                 result.push_back( value.first);
                                 
                              return result;
                           });

                           algorithm::container::trim( reply.content.services, algorithm::unique( algorithm::sort( reply.content.services)));

                           log::line( verbose::log, "reply: ", reply);
                           communication::device::blocking::optional::send( message.process.ipc, reply);
                        };
                     }
                  } // external::needs

               } // domain::discovery


               //! Handles ACK from services.
               //!
               //! if there are pending request for the "acked-service" we
               //! send response directly
               auto ack( State& state)
               {
                  return [&state]( const common::message::service::call::ACK& message)
                  {
                     Trace trace{ "service::manager::handle::local::ack"};
                     log::line( verbose::log, "message: ", message);

                     // we remove possible deadline first.
                     if( auto deadline = state.pending.deadline.remove( message.correlation))
                        signal::timer::set( deadline.value());

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
                           log::line( verbose::log, "found pendig: ", *found);

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
                           eventually::send( message.process, reply);
                        };
                     }
                  } // update

                  auto request( const State& state)
                  {
                     return [&state]( casual::configuration::message::Request& message)
                     {
                        Trace trace{ "service::manager::handle::local::configuration::request"};
                        log::line( verbose::log, "message: ", message);

                        auto reply = message::reverse::type( message);

                        reply.model.service = transform::configuration( state);
                        eventually::send( message.process, reply);
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

      } // handle

      handle::dispatch_type handler( State& state)
      {
         return {
            common::message::handle::defaults( ipc::device()),
            common::event::listener( 
               handle::local::event::process::exit( state)
            ),
            common::message::internal::dump::state::handle( state),
            handle::local::process::prepare::shutdown( state),
            handle::local::service::advertise( state),
            handle::local::service::lookup( state),
            handle::local::service::discard::lookup( state),
            handle::local::service::concurrent::advertise( state),
            handle::local::service::concurrent::metric( state),
            handle::local::ack( state),
            handle::local::event::subscription::begin( state),
            handle::local::event::subscription::end( state),
            handle::local::Call{ admin::services( state), state},
            handle::local::domain::discovery::request( state),
            handle::local::domain::discovery::reply( state),
            handle::local::domain::discovery::external::needs::request( state),
            handle::local::configuration::update::request( state),
            handle::local::configuration::request( state),
            handle::local::shutdown::request( state),
         };
      }

   } // service::manager
} // casual
