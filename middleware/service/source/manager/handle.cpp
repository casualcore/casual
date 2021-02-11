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
#include "common/signal/timer.h"
#include "common/environment.h"
#include "common/environment/normalize.h"
#include "common/algorithm.h"
#include "common/process.h"
#include "common/message/dispatch.h"
#include "common/message/handle.h"
#include "common/event/listen.h"
#include "common/event/send.h"

#include "common/communication/instance.h"

#include "configuration/message.h"

#include "domain/pending/message/send.h"

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
                        auto code = exception::code();
                        if( code != code::casual::communication_unavailable)
                           throw;
                           
                        log::line( log, code, " destination unavailable - ", destination);
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
                  auto send( std::vector< std::string> services, const Uuid& correlation = uuid::make())
                  {
                     Trace trace{ "service::manager::handle::local::discovery::send"};

                     common::message::gateway::domain::discover::Request request{ common::process::handle()};
                     request.correlation = correlation;
                     request.domain = common::domain::identity();
                     request.services = std::move( services);

                     log::line( verbose::log, "request: ", request);

                     auto& gateway = communication::instance::outbound::gateway::manager::optional::device();

                     return communication::device::blocking::optional::send( gateway, std::move( request));
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
                        state.services.emplace( route, result);

                     state.routes.emplace( service.name, std::move( service.routes));
                  }
               };

               algorithm::for_each( model.service.services, add_service);

               // accumulate restriction information
               for( auto& server : model.domain.servers)
               {
                  if( ! server.restrictions.empty())
                     algorithm::append_unique( server.restrictions, state.restrictions[ server.alias]);
               }

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
                              if( found->second.state() == state::instance::Sequential::State::busy)
                              {
                                 auto& instance = found->second;
                                 log::line( common::log::category::error, code::casual::invalid_semantics, 
                                    " callee terminated with pending reply to caller - callee: ", 
                                    event.state.pid, " - caller: ", instance.caller().process.pid);

                                 log::line( common::log::category::verbose::error, "instance: ", instance);

                                 local::error::reply( instance.caller(), common::code::xatmi::service_error);
                              }

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

                  namespace discoverable
                  {
                     auto available( State& state)
                     {
                        return [&state]( const common::message::event::discoverable::Avaliable& event)
                        {
                           Trace trace{ "service::manager::handle::local::event::discoverable::available"};
                           common::log::line( verbose::log, "event: ", event);

                           auto services = algorithm::transform( state.pending.lookups, []( auto& pending){ return pending.request.requested;});

                           algorithm::trim( services, algorithm::unique( algorithm::sort( services)));

                           if( ! services.empty())
                              local::discovery::send( std::move( services));
                        };
                     }
                     
                  } // discoverable

               } // event

               namespace process
               {
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

                           std::vector< common::Uuid> correlations;

                           algorithm::for_each( message.metrics, [&]( auto& metric)
                           {
                              correlations.push_back( metric.correlation);

                              if( auto service = state.service( metric.service))
                                 service->metric.update( metric);
                           });

                           if( auto instance = state.concurrent( message.process.pid))
                              instance->unreserve( correlations);

                           if( auto deadline = state.pending.deadline.remove( correlations))
                              signal::timer::set( deadline.value());

                           if( state.events)
                           {
                              state.metric.add( std::move( message.metrics));
                              handle::metric::batch::send( state);
                           }

                           log::line( verbose::log, "state.pending.deadline: ", state.pending.deadline);

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

                              
                              if( service->timeout.duration > platform::time::unit::zero() || ( ! service->timeout.duration && message.deadline))
                              {
                                 auto now = platform::time::clock::type::now();

                                 auto deadline = [&]()
                                 {
                                    if( service->timeout.duration > platform::time::unit::zero())
                                       return now + service->timeout.duration.value();
                                    return message.deadline.value();
                                 }();
                                 
                                 reply.service.timeout.duration = deadline - now;

                                 auto next = state.pending.deadline.add( {
                                    deadline,
                                    message.correlation,
                                    handle.pid,
                                    service});

                                 if( next)
                                    signal::timer::set( next.value() - now);
                              }
                             

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

                           if( auto found = algorithm::find( state.pending.lookups, message.correlation))
                           {
                              auto pending = algorithm::extract( state.pending.lookups, std::begin( found));

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
                           // else we've sent a discover request triggered by an discoverable event.
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
                     Trace trace{ "service::manager::handle::local::ack"};
                     log::line( verbose::log, "message: ", message);

                     // we remove possible deadline first.
                     if( auto deadline = state.pending.deadline.remove( message.correlation))
                        signal::timer::set( deadline.value());

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

                           if( auto found = common::algorithm::find_if( state.pending.lookups, has_pending))
                           {
                              log::line( verbose::log, "found pendig: ", *found);

                              auto pending = algorithm::extract( state.pending.lookups, std::begin( found));

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


               namespace configuration
               {
                  namespace update
                  {
                     auto request( State& state)
                     {
                        return []( casual::configuration::message::update::Request& message)
                        {
                           Trace trace{ "service::manager::handle::local::configuration::update::request"};
                           log::line( verbose::log, "message: ", message);


                           auto reply = message::reverse::type( message);
                           eventually::send( message.process, reply);

                        };
                     }
                  } // update

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

                  void transaction(
                        const common::transaction::ID& trid,
                        const common::server::Service& service,
                        const platform::time::unit& timeout,
                        const platform::time::point::type& now)
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
               handle::local::event::process::exit( state),
               handle::local::event::discoverable::available( state)
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
            handle::local::Call{ admin::services( state), state},
            handle::local::domain::discover::request( state),
            handle::local::domain::discover::reply( state),
            handle::local::configuration::update::request( state),
            handle::local::shutdown::request( state),
         };
      }

   } // service::manager
} // casual
