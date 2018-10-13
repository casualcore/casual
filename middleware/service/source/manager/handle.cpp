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
            const common::communication::ipc::Helper& device()
            {
               static communication::ipc::Helper singleton{ communication::error::handler::callback::on::Terminate{ &handle::process_exit}};
               return singleton;
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
                        try
                        {
                           ipc::device().blocking_send( device, message);
                           return true;
                        }
                        catch( const common::exception::system::communication::Unavailable&)
                        {
                           return false;
                        }
                     }
                  } // optional

                  namespace eventually
                  {
                     namespace detail
                     {
                        namespace get
                        {
                           template< typename D>
                           using is_process =  std::is_same< traits::remove_cvref_t< D>, common::process::Handle>;

                           template< typename D>
                           std::enable_if_t< is_process< D>::value, const common::process::Handle&>
                           process( D&& device) { return device;} 

                           template< typename D> 
                           std::enable_if_t< ! is_process< D>::value, const common::process::Handle&>
                           process( D&& device) { return device.connector().process();}

                           template< typename P, typename = std::enable_if_t< is_process< P>::value, strong::ipc::id>>
                           decltype( auto) device( P&& process) { return process.ipc;}
                           
                           template< typename D, typename = std::enable_if_t< ! is_process< D>::value>>
                           decltype( auto) process( D&& device) { return std::forward< D>( device);}

                        } // get
                     } // detail
                     template< typename D, typename M>
                     void send( State& state, D&& device, M&& message)
                     {
                        Trace trace{ "service::manager::handle::local::eventually::send"};

                        log::line( verbose::log, "message: ", message);

                        try
                        {
                           if( ! communication::ipc::non::blocking::send( detail::get::device( device), message, ipc::device().error_handler()))
                           {
                              log::line( log, "non blocking send failed - action: try later");

                              state.pending.replies.emplace_back( std::forward< M>( message), detail::get::process( device));
                           }
                        }
                        catch( const common::exception::system::communication::Unavailable&)
                        {
                           log::line( log, "destination unavailable - ", device);
                        }

                     }
                  } // eventually


               } // <unnamed>
            } // local


            void process_exit( const common::process::lifetime::Exit& exit)
            {
               //
               // We put a dead process event on our own ipc device, that
               // will be handled later on.
               //
               common::message::event::process::Exit event{ exit};
               communication::ipc::inbound::device().push( std::move( event));
            }



            namespace process
            {
               namespace local
               {
                  namespace
                  {
                     template< typename I>
                     void service_call_error_reply( State& state, const I& instance)
                     {
                        Trace trace{ "service::manager::handle::process::local::service_call_error_reply"};

                        log::line( common::log::category::error, "callee terminated with pending reply to caller - callee: ", 
                           instance.process.pid, " - caller: ", instance.caller().pid);

                        log::line( common::log::category::verbose::error, "instance: ", instance);

                        common::message::service::call::Reply message;
                        message.correlation = instance.correlation();
                        message.status = common::code::xatmi::service_error; 

                        handle::local::eventually::send( state, instance.caller(), std::move( message));
                     }

                  } // <unnamed>
               } // local

               void Exit::operator () ( message_type& message)
               {
                  Trace trace{ "service::manager::handle::process::Exit"};

                  log::line( verbose::log, "message: ", message);

                  //
                  // we need to check if the dead process has anyone wating for a reply
                  if( auto found = common::algorithm::find( m_state.instances.sequential, message.state.pid))
                  {
                     if( found->second.state() == state::instance::Sequential::State::busy)
                        local::service_call_error_reply( m_state, found->second);
                  }

                  m_state.remove_process( message.state.pid);
               }

               namespace prepare
               {
                  void Shutdown::operator () ( common::message::domain::process::prepare::shutdown::Request& message)
                  {
                     Trace trace{ "service::manager::handle::process::prepare::Shutdown"};

                     log::line( verbose::log, "message: ", message);

                     auto reply = common::message::reverse::type( message);
                     reply.process = common::process::handle();
                     reply.processes = std::move( message.processes);

                     algorithm::for_each( reply.processes, [&]( const auto& p){
                        m_state.prepare_shutdown( p.pid);
                     });


                     ipc::device().blocking_send( message.process.ipc, reply);
                  }

               } // prepare

            } // process


            namespace event
            {
               namespace subscription
               {
                  void Begin::operator () ( common::message::event::subscription::Begin& message)
                  {
                     Trace trace{ "service::manager::handle::event::subscription::Begin"};

                     log::line( verbose::log, "message: ", message);

                     m_state.events.subscription( message);
                  }

                  void End::operator () ( message_type& message)
                  {
                     Trace trace{ "service::manager::handle::event::subscription::End"};

                     log::line( verbose::log, "message: ", message);

                     m_state.events.subscription( message);
                  }


               } // subscription

            } // event


            namespace service
            {
               void Advertise::operator () ( message_type& message)
               {
                  Trace trace{ "service::manager::handle::service::Advertise"};

                  log::line( verbose::log, "message: ", message);

                  m_state.update( message);
               }

               namespace concurrent
               {
                  void Advertise::operator () ( message_type& message)
                  {
                     Trace trace{ "service::manager::handle::service::concurrent::Advertise"};

                     common::log::line( verbose::log, "message: ", message);

                     m_state.update( message);
                  }

                  void Metric::operator () ( common::message::service::concurrent::Metric& message)
                  {
                     Trace trace{ "service::manager::handle::service::concurrent::Metric"};

                     log::line( verbose::log, "message: ", message);

                     auto now = platform::time::clock::type::now();

                     for( auto& s : message.services)
                     {
                        auto service = m_state.find_service( s.name);
                        if( service)
                        {
                           service->metric.add( s.duration);

                           // TODO: do we need more accuracy?
                           service->last( now);
                        }
                     }
                  }
               } // concurrent


               void Lookup::operator () ( message_type& message)
               {
                  Trace trace{ "service::manager::handle::service::Lookup"};

                  auto now = platform::time::clock::type::now();

                  log::line( verbose::log, "message: ", message);

                  try
                  {
                     auto& service = m_state.service( message.requested);

                     auto handle = service.reserve( now, message.process, message.correlation);

                     if( handle)
                     {

                        auto reply = common::message::reverse::type( message);
                        reply.service = service.information;
                        reply.service.event_subscribers = m_state.subscribers();
                        reply.state = decltype( reply.state)::idle;
                        reply.process = handle;

                        local::optional::send( message.process.ipc, reply);
                     }
                     else if( ! service.instances.active())
                     {
                        throw state::exception::Missing{ "no instances"};
                     }
                     else
                     {
                        auto reply = common::message::reverse::type( message);
                        reply.service = service.information;
                        reply.service.event_subscribers = m_state.subscribers();

                        switch( message.context)
                        {
                           case common::message::service::lookup::Request::Context::no_reply:
                           case common::message::service::lookup::Request::Context::forward:
                           {
                              //
                              // The intention is "send and forget", or a plain forward, we use our forward-cache for this
                              //
                              reply.process = m_state.forward;

                              //
                              // Caller will think that service is idle, that's the whole point
                              // with our forward.
                              //
                              reply.state = decltype( reply.state)::idle;

                              local::optional::send( message.process.ipc, reply);

                              break;
                           }
                           case common::message::service::lookup::Request::Context::gateway:
                           {
                              //
                              // the request is from some other domain, we'll wait until
                              // the service is idle. That is, we don't need to send timeout and
                              // stuff to the gateway, since the other domain has provided this to
                              // the caller (which of course can differ from our timeouts, if operation
                              // has change the timeout, TODO: something we need to address? probably not,
                              // since we can't make it 100% any way...)
                              //
                              m_state.pending.requests.emplace_back( std::move( message), now);

                              break;
                           }
                           default:
                           {
                              // send busy-message to caller, to set timeouts and stuff
                              reply.state = decltype( reply.state)::busy;

                              if( local::optional::send( message.process.ipc, reply))
                              {
                                 // All instances are busy, we stack the request
                                 m_state.pending.requests.emplace_back( std::move( message), now);
                              }

                              break;
                           }
                        }
                     }
                  }
                  catch( const state::exception::Missing&)
                  {

                     auto send_reply = execute::scope( [&](){

                        log::line( log, "no instances found for service: ", message.requested);

                        //
                        // Server (queue) that hosts the requested service is not found.
                        // We propagate this by having absent state
                        //
                        auto reply = common::message::reverse::type( message);
                        reply.service.name = message.requested;
                        reply.state = decltype( reply.state)::absent;

                        local::optional::send( message.process.ipc, reply);
                     });

                     try
                     {

                        common::message::gateway::domain::discover::Request request;
                        request.correlation = message.correlation;
                        request.domain = common::domain::identity();
                        request.process = common::process::handle();
                        request.services.push_back( message.requested);

                        // If there is no gateway, this will throw
                        ipc::device().blocking_send( communication::instance::outbound::gateway::manager::optional::device(), request);

                        log::line( log, "no instances found for service: ", message.requested, " - action: ask neighbor domains");

                        m_state.pending.requests.emplace_back( std::move( message), now);

                        send_reply.release();
                     }
                     catch( ...)
                     {
                        common::exception::handle();
                     }
                  }
               }
            } // service

            namespace domain
            {


               namespace discover
               {
                  void Request::operator () ( message_type& message)
                  {
                     Trace trace{ "service::manager::handle::domain::discover::Request"};

                     common::log::line( verbose::log, "message: ", message);

                     auto reply = common::message::reverse::type( message);

                     reply.process = common::process::handle();
                     reply.domain = common::domain::identity();

                     for( const auto& s : message.services)
                     {
                        auto&& service = m_state.find_service( s);

                        // We don't allow other domains to access or know about our
                        // admin services.
                        if( service && service->information.category != common::service::category::admin())
                        {
                           if( ! service->instances.sequential.empty())
                           {
                              // Service is local
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
                     }

                     ipc::device().blocking_send( message.process.ipc, reply);
                  }

                  void Reply::operator () ( message_type& message)
                  {
                     Trace trace{ "service::manager::handle::gateway::discover::Reply"};

                     common::log::line( verbose::log, "message: ", message);

                     auto found = algorithm::find_if( m_state.pending.requests, [&]( const state::service::Pending& p){
                        return p.request.correlation == message.correlation;
                     });

                     if( found)
                     {
                        auto pending = std::move( *found);
                        m_state.pending.requests.erase( std::begin( found));

                        auto service = m_state.find_service( pending.request.requested);

                        if( service && ! service->instances.empty())
                        {
                           //
                           // The requested service is now available, use
                           // the lookup to decide how to progress.
                           //
                           service::Lookup{ m_state}( pending.request);
                        }
                        else
                        {
                           auto reply = common::message::reverse::type( pending.request);
                           reply.service.name = pending.request.requested;
                           reply.state = decltype( reply.state)::absent;

                           ipc::device().blocking_send( pending.request.process.ipc, reply);
                        }

                     }
                     else
                     {
                        log::line( log, "failed to correlate pending request - assume it has been consumed by a recent started local server");
                     }
                  }


               } // discover

            } // domain


            void ACK::operator () ( message_type& message)
            {
               Trace trace{ "service::manager::handle::ACK"};

               log::line( verbose::log, "message: ", message);

               try
               {
                  auto now = platform::time::clock::type::now();

                  // This message can only come from a local instance
                  auto& instance = m_state.local( message.process.pid);
                  auto service = instance.unreserve( now);

                  //
                  // Check if there are pending request for services that this
                  // instance has.
                  //

                  {
                     auto pending = common::algorithm::find_if( m_state.pending.requests, [&]( const auto& p){
                        return instance.service( p.request.requested);
                     });

                     if( pending)
                     {
                        log::line( verbose::log, "found pendig: ", *pending);

                        //
                        // We now know that there are one idle server that has advertised the
                        // requested service (we've just marked it as idle...).
                        // We can use the normal request to get the response
                        //
                        service::Lookup lookup( m_state);
                        lookup( pending->request);

                        // add pending metrics
                        service->pending.add( now - pending->when);

                        // Remove pending
                        m_state.pending.requests.erase( std::begin( pending));
                     }
                  }
               }
               catch( const state::exception::Missing&)
               {
                  log::line( log::category::error, "failed to find instance on ACK - indicate inconsistency - action: ignore");
               }
            }


            void Policy::configure( common::server::Arguments& arguments)
            {
               m_state.connect_manager( arguments.services);
            }


            void Policy::reply( common::strong::ipc::id id, common::message::service::call::Reply& message)
            {
               ipc::device().blocking_send( id, message);
            }

            void Policy::ack()
            {
               common::message::service::call::ACK ack;

               ack.process = common::process::handle();

               ACK sendACK( m_state);
               sendACK( ack);
            }

            void Policy::transaction(
                  const common::transaction::ID& trid,
                  const common::server::Service& service,
                  const common::platform::time::unit& timeout,
                  const common::platform::time::point::type& now)
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
               throw common::exception::xatmi::System{ "can't forward within service-manager"};
            }

            void Policy::statistics( common::strong::ipc::id, common::message::event::service::Call&)
            {
               // We don't collect statistics for the service-manager
            }

         } // handle

         handle::dispatch_type handler( State& state)
         {
            return {
               common::event::listener( handle::process::Exit{ state}),
               handle::process::prepare::Shutdown{ state},
               handle::service::Advertise{ state},
               handle::service::Lookup{ state},
               handle::service::concurrent::Advertise{ state},
               handle::service::concurrent::Metric{ state},
               handle::ACK{ state},
               handle::event::subscription::Begin{ state},
               handle::event::subscription::End{ state},
               handle::Call{ admin::services( state), state},
               handle::domain::discover::Request{ state},
               handle::domain::discover::Reply{ state},
               common::message::handle::Ping{},
               common::message::handle::Shutdown{},
            };
         }
      } // manager
   } // service
} // casual
