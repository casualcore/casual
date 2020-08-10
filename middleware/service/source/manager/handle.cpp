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
                              Trace trace{ "service::manager::handle::process::local::service_call_error_reply"};

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
                           Trace trace{ "service::manager::handle::process::Exit"};
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
                     auto advertise( State& state)
                     {
                        return [&state]( common::message::service::Advertise& message)
                        {
                           Trace trace{ "service::manager::handle::service::Advertise"};
                           log::line( verbose::log, "message: ", message);

                           state.update( message);
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

                              state.update( message);
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
                                 if( auto service = state.find_service( metric.service))
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
                           Trace trace{ "service::manager::handle::service::Lookup::discover"};

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

                           try
                           {
                              log::line( log, "no instances found for service: ", name, " - action: ask neighbor domains");

                              common::message::gateway::domain::discover::Request request;
                              request.correlation = message.correlation;
                              request.domain = common::domain::identity();
                              request.process = common::process::handle();
                              request.services.push_back( name);

                              // If there is no gateway, this will throw
                              communication::device::blocking::send( communication::instance::outbound::gateway::manager::optional::device(), request);

                              state.pending.requests.emplace_back( std::move( message), platform::time::clock::type::now());

                              send_reply.release();
                           }
                           catch( ...)
                           {
                              common::exception::code();
                           }
                        }

                        void lookup( State& state, common::message::service::lookup::Request& message, platform::time::unit pending)
                        {
                           Trace trace{ "service::manager::handle::service::Lookup::lookup"};
                           log::line( verbose::log, "message: ", message, ", pending: ", pending);

                           try
                           {
                              // will throw 'Missing' if not found, and a discover will take place
                              auto& service = state.service( message.requested);

                              auto handle = service.reserve( message.process, message.correlation);

                              if( handle)
                              {
                                 auto reply = common::message::reverse::type( message);
                                 reply.service = service.information;
                                 reply.state = decltype( reply.state)::idle;
                                 reply.process = handle;
                                 reply.pending = pending;

                                 local::optional::send( message.process.ipc, reply);
                              }
                              else if( service.instances.empty())
                              {
                                 // note: vi discover on the "real service name", in case it's a route
                                 discover( state, std::move( message), service.information.name);
                              }
                              else
                              {
                                 auto reply = common::message::reverse::type( message);
                                 reply.service = service.information;

                                 switch( message.context)
                                 {
                                    case common::message::service::lookup::Request::Context::no_reply:
                                    case common::message::service::lookup::Request::Context::forward:
                                    {
                                       // The intention is "send and forget", or a plain forward, we use our forward-cache for this
                                       reply.process = state.forward;

                                       // Caller will think that service is idle, that's the whole point
                                       // with our forward.
                                       reply.state = decltype( reply.state)::idle;

                                       local::optional::send( message.process.ipc, reply);

                                       break;
                                    }
                                    case common::message::service::lookup::Request::Context::gateway:
                                    {
                                       // the request is from some other domain, we'll wait until
                                       // the service is idle. That is, we don't need to send timeout and
                                       // stuff to the gateway, since the other domain has provided this to
                                       // the caller (which of course can differ from our timeouts, if operation
                                       // has change the timeout) 
                                       // TODO semantics: something we need to address? probably not,
                                       // since we can't make it 100% any way...)
                                       state.pending.requests.emplace_back( std::move( message), platform::time::clock::type::now());

                                       break;
                                    }
                                    default:
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
                                 }
                              }
                           }
                           catch( ...)
                           {
                              auto code = exception::code();
                              if( code != code::casual::domain_instance_unavailable)
                                 throw;

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

                              auto equal_correlation = [&message]( auto& pending)
                              {
                                 return message.correlation == pending.request.correlation;
                              };

                              auto reply = message::reverse::type( message);

                              if( auto found = algorithm::find_if( state.pending.requests, equal_correlation))
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
                                 auto service = state.find_service( name);

                                 // We don't allow other domains to access or know about our
                                 // admin services.
                                 if( service && service->information.category != common::service::category::admin)
                                 {
                                    if( ! service->instances.sequential.empty())
                                    {
                                       reply.services.emplace_back(
                                             service->information.name,
                                             service->information.category,
                                             service->information.transaction);
                                    }
                                    else if( ! service->instances.concurrent.empty())
                                    {
                                       reply.services.emplace_back(
                                             service->information.name,
                                             service->information.category,
                                             service->information.transaction,
                                             service->instances.concurrent.front().hops());
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

                              auto has_correlation = [&message]( auto& pending)
                              {
                                 return pending.request.correlation == message.correlation;
                              };

                              if( auto found = algorithm::find_if( state.pending.requests, has_correlation))
                              {
                                 auto pending = std::move( *found);
                                 state.pending.requests.erase( std::begin( found));

                                 auto service = state.find_service( pending.request.requested);

                                 if( service && ! service->instances.empty())
                                 {
                                    // The requested service is now available, use
                                    // the lookup to decide how to progress.
                                    service::lookup( state)( pending.request);
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
                        Trace trace{ "service::manager::handle::ACK"};
                        log::line( verbose::log, "message: ", message);

                        try
                        {
                           //auto now = platform::time::clock::type::now();

                           // This message can only come from a local instance
                           auto& instance = state.sequential( message.metric.process.pid);
                           auto service = instance.unreserve( message.metric);

                           // add metric
                           if( state.events.active< common::message::event::service::Calls>())
                           {
                              state.metric.add( std::move( message.metric));
                              handle::metric::batch::send( state);
                           }

                           // Check if there are pending request for services that this
                           // instance has.

                           {
                              auto has_pending = [&instance]( const auto& pending)
                              {
                                 return instance.service( pending.request.requested);
                              };

                              if( auto pending = common::algorithm::find_if( state.pending.requests, has_pending))
                              {
                                 log::line( verbose::log, "found pendig: ", *pending);

                                 const auto waited = platform::time::clock::type::now() - pending->when;

                                 // We now know that there are one idle server that has advertised the
                                 // requested service (we've just marked it as idle...).
                                 // We can use the normal request to get the response
                                 service::detail::lookup( state, pending->request, waited);

                                 // add pending metrics
                                 service->metric.pending += waited;

                                 // Remove pending
                                 state.pending.requests.erase( std::begin( pending));
                              }
                           }
                        }
                        catch( ...)
                        {
                           auto code = exception::code();
                           if( code != code::casual::domain_instance_unavailable)
                              throw;

                           log::line( log::category::error, code, " failed to find instance on ACK - indicate inconsistency - action: ignore");
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
