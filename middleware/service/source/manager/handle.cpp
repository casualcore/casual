//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "service/manager/handle.h"
#include "service/manager/admin/server.h"
#include "service/transform.h"
#include "service/common.h"

#include "common/server/lifetime.h"
#include "common/environment.h"
#include "common/algorithm.h"
#include "common/process.h"
#include "common/message/dispatch.h"
#include "common/message/handle.h"
#include "common/event/listen.h"

#include "common/communication/instance.h"

#include "domain/pending/message/send.h"

// std
#include <vector>
#include <string>

namespace casual
{
   using namespace common;

   namespace service
   {
      namespace manager
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
                     bool send( D&& device, M&& message)
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
                           auto code = exception::code();
                           if( code != code::casual::communication_unavailable)
                              throw;
                              
                           log::line( log, code, " destination unavailable - ", destination);
                           return false;
                        }

                     }
                  } // eventually

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
               } // <unnamed>
            } // local

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
                  namespace process
                  {
                     namespace detail
                     {
                        namespace error
                        {
                           auto reply = []( State& state, auto& instance)
                           {
                              Trace trace{ "service::manager::handle::local::process::detail::error::reply"};

                              log::line( common::log::category::error, code::casual::invalid_semantics, 
                                 " callee terminated with pending reply to caller - callee: ", 
                                 instance.process.pid, " - caller: ", instance.caller().pid);

                              log::line( common::log::category::verbose::error, "instance: ", instance);

                              common::message::service::call::Reply message;
                              message.correlation = instance.correlation();
                              message.code.result = common::code::xatmi::service_error; 

                              handle::local::eventually::send( instance.caller(), std::move( message));
                           };
                        } // error
                     } // detail

                     auto exit( State& state)
                     {
                        return [&state]( common::message::event::process::Exit& message)
                        {
                           Trace trace{ "service::manager::handle::local::process::detail::exit"};
                           log::line( verbose::log, "message: ", message);

                           // we need to check if the dead process has anyone wating for a reply
                           if( auto found = common::algorithm::find( state.instances.sequential, message.state.pid))
                              if( found->second.state() == state::instance::Sequential::State::busy)
                                 detail::error::reply( state, found->second);

                           state.remove( message.state.pid);
                        };
                     }

                     namespace prepare
                     {
                        auto shutdown( State& state)
                        {
                           return [&state]( common::message::domain::process::prepare::shutdown::Request& message)
                           {
                              Trace trace{ "service::manager::handle::process::prepare::Shutdown"};
                              log::line( verbose::log, "message: ", message);

                              auto reply = common::message::reverse::type( message, common::process::handle());
                              reply.processes = std::move( message.processes);

                              auto deactivate_process = [&]( auto& process)
                              {
                                 state.deactivate( process.pid);
                              };

                              // all we need to do is to deactivate all servers (remove all 
                              // associated services). domain-manager will then send shutdown to
                              // the servers, and we'll receive a process::Exit when the server 
                              // has exit, and we'll clean up the rest.
                              algorithm::for_each( reply.processes, deactivate_process);

                              handle::local::eventually::send( message.process, reply);
                           };
                        }

                     } // prepare
                  } // process

                  namespace event
                  {
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
                           void pending( State& state, std::vector< state::service::Pending>&& pending);
                        } // handle

                        
                     } // detail

                     auto advertise( State& state)
                     {
                        return [&state]( common::message::service::Advertise& message)
                        {
                           Trace trace{ "service::manager::handle::service::Advertise"};
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
                              Trace trace{ "service::manager::handle::service::concurrent::Advertise"};
                              common::log::line( verbose::log, "message: ", message);

                              // some pending might got resolved.
                              detail::handle::pending( state, state.update( message));
                           };
                        }

                        auto metric( State& state)
                        {
                           return [&state]( common::message::event::service::Calls& message)
                           {
                              Trace trace{ "service::manager::handle::service::concurrent::Metric"};
                              log::line( verbose::log, "message: ", message);

                              auto update_metric = [&]( auto& metric)
                              {
                                 if( auto service = state.service( metric.service))
                                    service->metric.update( metric);
                              };

                              algorithm::for_each( message.metrics, update_metric);

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

                              common::message::gateway::domain::discover::Request request;
                              request.correlation = message.correlation;
                              request.domain = common::domain::identity();
                              request.process = common::process::handle();
                              request.services.push_back( name);

                              auto& gateway = communication::instance::outbound::gateway::manager::optional::device();

                              if( communication::device::blocking::optional::send( gateway, request)
                                 || message.context == decltype( message.context)::wait)
                              {
                                 // we sent the request OR the caller is willing to wait for future 
                                 // advertised services

                                 state.pending.requests.emplace_back( std::move( message), platform::time::clock::type::now());
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
                              auto handle = service->reserve( message.process, message.correlation);

                              if( handle)
                              {
                                 auto reply = common::message::reverse::type( message);
                                 reply.service = service->information;
                                 reply.state = decltype( reply.state)::idle;
                                 reply.process = handle;
                                 reply.pending = pending;

                                 // TODO maintainence: if the message is not sent, we need to unreserve
                                 local::optional::send( message.process.ipc, reply);
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
                                       state.pending.requests.emplace_back( std::move( message), platform::time::clock::type::now());

                                       break;
                                    }
                                    case Context::regular:
                                    {
                                       // send busy-message to caller, to set timeouts and stuff
                                       reply.state = decltype( reply.state)::busy;

                                       if( local::optional::send( message.process.ipc, reply))
                                       {
                                          // All instances are busy, we stack the request
                                          state.pending.requests.emplace_back( std::move( message), platform::time::clock::type::now());
                                       }

                                       break;
                                    }
                                    case Context::wait:
                                    {
                                       // we know the service exists, and it got instances, we do a regular pending.
                                       state.pending.requests.emplace_back( std::move( message), platform::time::clock::type::now());
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

                              if( auto found = algorithm::find( state.pending.requests, message.correlation))
                              {
                                 log::line( log, "found pending to discard");
                                 log::line( verbose::log, "pending: ", *found);

                                 state.pending.requests.erase( std::begin( found));
                                 reply.state = decltype( reply.state)::discarded;
                              }
                              else 
                              {
                                 log::line( log, "failed to find pending to discard - check if we have reserved the service already");

                                 // we need to go through all instances.

                                 auto reserved_service = [&message]( auto& tuple)
                                 {
                                    auto& instance = tuple.second;
                                    return ! instance.idle() && instance.correlation() == message.correlation;
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

                              // We allways send reply
                              local::optional::send( message.process.ipc, reply);
                           };
                        }
                     } // discard

                     namespace detail
                     {
                        namespace handle
                        {
                           // Used by advertised above, defined here to be able to use lookup...
                           void pending( State& state, std::vector< state::service::Pending>&& pending)
                           {
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
                              
                              // we just use the 'regular' lookup to take care of the pending
                              algorithm::for_each( pending, lookup);
                           }
                        } // handle

                        
                     } // detail
                  } // service

                  namespace domain
                  {
                     namespace discover
                     {
                        auto request( State& state)
                        {
                           return [&state]( common::message::gateway::domain::discover::Request& message)
                           {
                              Trace trace{ "service::manager::handle::domain::discover::Request"};
                              common::log::line( verbose::log, "message: ", message);

                              auto reply = common::message::reverse::type( message);

                              reply.process = common::process::handle();
                              reply.domain = common::domain::identity();

                              auto known_services = [&]( auto& name)
                              {
                                 auto service = state.service( name);

                                 // We don't allow other domains to access or know about our
                                 // admin services.
                                 if( service && service->information.category != common::service::category::admin)
                                 {
                                    if( ! service->instances.sequential.empty())
                                    {
                                       reply.services.emplace_back(
                                             name,
                                             service->information.category,
                                             service->information.transaction);
                                    }
                                    else if( ! service->instances.concurrent.empty())
                                    {
                                       reply.services.emplace_back(
                                             name,
                                             service->information.category,
                                             service->information.transaction,
                                             service->instances.concurrent.front().property);
                                    }
                                 }
                              };

                              algorithm::for_each( message.services, known_services);

                              communication::device::blocking::send( message.process.ipc, reply);
                           };
                        }

                        auto reply( State& state)
                        {
                           return [&state]( common::message::gateway::domain::discover::accumulated::Reply& message)
                           {
                              Trace trace{ "service::manager::handle::gateway::discover::Reply"};
                              common::log::line( verbose::log, "message: ", message);

                              if( auto found = algorithm::find( state.pending.requests, message.correlation))
                              {
                                 auto pending = algorithm::extract( state.pending.requests, std::begin( found));

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
                                    state.pending.requests.push_front( std::move( pending));
                                 }
                                 else 
                                 {
                                    auto reply = common::message::reverse::type( pending.request);
                                    reply.service.name = pending.request.requested;
                                    reply.state = decltype( reply.state)::absent;

                                    communication::device::blocking::send( pending.request.process.ipc, reply);
                                 }
                              }
                              else
                                 log::line( log, "failed to correlate pending request - assume it has been consumed by a recent started local server");
                           };
                        }

                     } // discover
                  } // domain

                  //! Handles ACK from services.
                  //!
                  //! if there are pending request for the "acked-service" we
                  //! send response directly
                  auto ack( State& state)
                  {
                     return [&state]( const common::message::service::call::ACK& message)
                     {
                        Trace trace{ "service::manager::local::handle::ack"};
                        log::line( verbose::log, "message: ", message);

                        // This message can only come from a local instance
                        if( auto instance = state.sequential( message.metric.process.pid))
                        {
                           instance->unreserve( message.metric);

                           // add event-metric
                           if( state.events.active< common::message::event::service::Calls>())
                           {
                              state.metric.add( std::move( message.metric));
                              handle::metric::batch::send( state);
                           }

                           // Check if there are pending request for services that this
                           // instance has.

                           {
                              auto has_pending = [instance]( const auto& pending)
                              {
                                 return instance->service( pending.request.requested);
                              };

                              if( auto found = common::algorithm::find_if( state.pending.requests, has_pending))
                              {
                                 log::line( verbose::log, "found pendig: ", *found);

                                 auto pending = algorithm::extract( state.pending.requests, std::begin( found));

                                 // We now know that there are one idle server that has advertised the
                                 // requested service (we've just marked it as idle...).
                                 // We can use the normal request to get the response
                                 service::detail::lookup( state, pending.request, platform::time::clock::type::now() - pending.when);
                              }
                           }
                        }
                        else
                        {
                           log::line( log::category::error, 
                              code::casual::internal_correlation, 
                              " failed to find instance on ACK - indicate inconsistency - action: ignore");
                        }
                     };
                  }
               } // <unnamed>
            } // local

            void Policy::configure( common::server::Arguments& arguments)
            {
               m_state.connect_manager( arguments.services);
            }

            void Policy::reply( common::strong::ipc::id id, common::message::service::call::Reply& message)
            {
               communication::device::blocking::send( id, message);
            }

            void Policy::ack( const common::message::service::call::ACK& ack)
            {
               local::ack( m_state)( ack);
            }

            void Policy::transaction(
                  const common::transaction::ID& trid,
                  const common::server::Service& service,
                  const platform::time::unit& timeout,
                  const platform::time::point::type& now)
            {
               // service-manager doesn't bother with transactions...
            }

            common::message::service::Transaction Policy::transaction( bool commit)
            {
               // service-manager doesn't bother with transactions...
               return {};
            }

            void Policy::forward( common::service::invoke::Forward&& forward, const common::message::service::call::callee::Request& message)
            {
               code::raise::error( code::casual::invalid_semantics, "can't forward within service-manager");
            }

            void Policy::statistics( common::strong::ipc::id, common::message::event::service::Call&)
            {
               // We don't collect statistics for the service-manager
            }
         } // handle

         handle::dispatch_type handler( State& state)
         {
            return {
               common::message::handle::defaults( ipc::device()),
               common::event::listener( handle::local::process::exit( state)),
               handle::local::process::prepare::shutdown( state),
               handle::local::service::advertise( state),
               handle::local::service::lookup( state),
               handle::local::service::discard::lookup( state),
               handle::local::service::concurrent::advertise( state),
               handle::local::service::concurrent::metric( state),
               handle::local::ack( state),
               handle::local::event::subscription::begin( state),
               handle::local::event::subscription::end( state),
               handle::Call{ admin::services( state), state},
               handle::local::domain::discover::request( state),
               handle::local::domain::discover::reply( state),
            };
         }
      } // manager
   } // service
} // casual
