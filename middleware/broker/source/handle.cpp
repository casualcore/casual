//!
//! handle.cpp
//!
//! Created on: Jun 1, 2014
//!     Author: Lazan
//!

#include "broker/handle.h"
#include "broker/transform.h"
#include "broker/filter.h"
#include "broker/admin/server.h"

#include "common/server/lifetime.h"
#include "common/queue.h"
#include "common/internal/log.h"
#include "common/environment.h"
#include "common/algorithm.h"
#include "common/process.h"
#include "common/message/dispatch.h"
#include "common/message/handle.h"


//
// std
//
#include <vector>
#include <string>
#include <fstream>

namespace casual
{
   using namespace common;

   namespace broker
   {

      namespace handle
      {
         namespace local
         {
            namespace
            {
               struct Boot : state::Base
               {
                  using state::Base::Base;

                  void operator () ( const state::Executable& executable)
                  {
                     try
                     {
                        handle::boot( m_state, executable, executable.configured_instances);
                     }
                     catch( const exception::invalid::Argument& exception)
                     {
                        log::error << "failed to boot executable: " << executable << " - why: " << exception << std::endl;
                     }
                  }


                  void operator () ( State::Batch& batch)
                  {
                     //
                     // If something throws, we shutdown...
                     //
                     common::scope::Execute scope_shutdown{ std::bind( &handle::send_shutdown, std::ref( m_state))};

                     common::log::information << "boot group '" << batch.group << "'\n";

                     common::range::for_each( batch.servers, *this);
                     common::range::for_each( batch.executables, *this);


                     queue::blocking::Reader queueReader{ common::ipc::receive::queue(), broker::handle::dead::Process{ m_state.ipc()}};

                     auto handler = broker::handler( m_state);

                     //
                     // Use a filter so we don't consume any incoming messages that
                     // we don't handle right now.
                     //
                     auto filter = handler.types();


                     while( ! common::range::all_of( batch.servers, filter::Booted{ m_state}))
                     {
                        try
                        {
                           common::signal::timer::Scoped timeout{ std::chrono::seconds( 10)};

                           auto marshal = queueReader.next( filter);
                           handler( marshal);
                        }
                        catch( const common::exception::signal::Timeout& exception)
                        {
                           common::log::error << "failed to get response from spawned instances in a timely manner - action: abort" << std::endl;
                           throw common::exception::signal::Terminate{};
                        }
                     }

                     //
                     // we're done, release the shutdown
                     //
                     scope_shutdown.release();
                  }
               };

               struct Shutdown : state::Base
               {
                  Shutdown( State& state, common::ipc::receive::Queue& ipc) : Base( state), m_ipc( ipc) {}


                  void operator () ( State::Batch& batch)
                  {
                     common::log::information << "shutdown group '" << batch.group << "'\n";


                     auto handler = broker::handler_no_services( m_state);

                     auto filter = handler.types();

                     queue::non_blocking::Reader non_blocking{ m_ipc, broker::handle::dead::Process{ m_ipc}};

                     //
                     // Take care of executables
                     //

                     for( auto& executable : batch.executables)
                     {
                        server::lifetime::shutdown( broker::handle::dead::Process{ m_ipc}, {}, { executable.get().instances}, std::chrono::seconds( 2));

                        while( handler( non_blocking.next( filter)))
                           ;
                     }


                     //
                     // Take care of xatmi-servers
                     //

                     for( auto& server : batch.servers)
                     {
                        auto pids = server.get().instances;

                        log::internal::debug << "shutdown pids: " << range::make( pids) << '\n';

                        for( auto pid : pids)
                        {
                           //
                           // Make sure we don't add our self
                           //
                           if( pid != process::id())
                           {
                              server::lifetime::shutdown( broker::handle::dead::Process{ m_ipc}, { m_state.getInstance( pid).process}, {}, std::chrono::seconds( 2));

                              while( handler( non_blocking.next()))
                                 ;
                           }
                        }
                     }
                  }
               private:
                  common::ipc::receive::Queue& m_ipc;
               };


               namespace handle
               {
                  void error()
                  {
                     try
                     {
                        throw;
                     }
                     catch( const state::exception::Missing& exception)
                     {
                        log::error << exception << " - action: discard";
                     }
                  }


                  template< typename M, typename R>
                  bool connect( State& state, M&& message, R&& reply)
                  {
                     queue::blocking::Send send{ broker::handle::dead::Process{ state.ipc()}};

                     reply.correlation = message.correlation;

                     if( state.mode == State::Mode::shutdown)
                     {
                        log::error << "application connect while in 'shutdown-mode' - instance: " << message.process << " - action: reply with 'shutdown-error'\n";

                        //
                        // We're in a shtudown mode, we don't allow any connections
                        //
                        reply.directive = R::Directive::shutdown;
                        send( message.process.queue, reply);

                        return false;
                     }
                     else if( message.identification)
                     {
                        //
                        // Make sure we only start one instance of a singleton
                        //

                        auto found = common::range::find( state.singeltons, message.identification);

                        if( found)
                        {
                           if( found->second != message.process)
                           {
                              log::error << "only one instance of application with id " << message.identification << " is allowed - instance: " << message.process << "- action: reply with 'singleton-error'\n";

                              reply.directive = R::Directive::singleton;

                              queue::blocking::Send send{ broker::handle::dead::Process{ state.ipc()}};
                              send( message.process.queue, reply);

                              auto& server = state.getServer( state.getInstance( message.process.pid).server);
                              server.configured_instances = 1;

                              return false;
                           }
                        }
                        else
                        {
                           state.singeltons[ message.identification] = message.process;

                           decltype( state.pending.process_lookup) pending;
                           std::swap( pending, state.pending.process_lookup);

                           range::for_each( pending, broker::handle::lookup::Process{ state});
                        }
                     }

                     common::log::internal::debug << "connect reply: " << message.process << std::endl;

                     send( message.process.queue, reply);

                     return true;
                  }

               } // handle

            } // <unnamed>
         } // local

         void boot( State& state)
         {

            auto boot_order = state.bootOrder();

            range::for_each( boot_order, local::Boot{ state});

            if( log::internal::debug)
            {
               std::vector< std::reference_wrapper< state::Server>> servers;

               for( auto& s : state.servers)
               {
                  servers.emplace_back( s.second);
               }
               log::internal::debug << "booted servers: " << range::make( servers) << '\n';
            }
         }


         void shutdown( State& state, common::ipc::receive::Queue& ipc)
         {
            auto shutdown_order = range::reverse( state.bootOrder());

            try
            {
               signal::timer::Scoped alarm( std::chrono::seconds( 10));

               range::for_each( shutdown_order, local::Shutdown{ state, ipc});
            }
            catch( const exception::signal::Timeout&)
            {
               log::error << "failed to shutdown - TODO: send reply" << std::endl;
            }
         }

         void send_shutdown( State& state)
         {
            queue::non_blocking::Send send{ broker::handle::dead::Process{ state.ipc()}};

            common::queue::non_blocking::force::send(
               common::ipc::receive::queue(),
               send,
               common::message::shutdown::Request{});
         }


         std::vector< common::platform::pid_type> spawn( const State& state, const state::Executable& executable, std::size_t instances)
         {
            std::vector< common::platform::pid_type> pids;

            //
            // Prepare environment. We use the default first and add
            // specific for the executable
            //
            auto environment = state.standard.environment;
            environment.insert(
               std::end( environment),
               std::begin( executable.environment.variables),
               std::end( executable.environment.variables));

            while( instances-- > 0)
            {
               pids.push_back( common::process::spawn( executable.path, executable.arguments, environment));
            }

            return pids;
         }

         void boot( State& state, const state::Executable& executable, std::size_t instances)
         {
            instances = std::max( executable.configured_instances, instances);

            auto count = instances - executable.instances.size();

            if( count > 0)
            {
               state.addInstances( executable.id, spawn( state, executable, count));
            }
         }

         void shutdown( State& state, const state::Server& server, std::size_t instances)
         {
            Trace trace{ "broker::handle::shutdown", log::internal::debug};

            instances = std::min( server.instances.size(), instances);

            auto range = common::range::make( server.instances);

            range.first = range.last - instances;


            for( auto& pid : range)
            {
               //
               // make sure we don't try to shutdown our self...
               //
               if( pid != common::process::id())
               {
                  try
                  {
                     auto& instance = state.getInstance( pid);
                     instance.alterState( state::Server::Instance::State::shutdown);

                     queue::non_blocking::Send sender{ handle::dead::Process{ state.ipc()}};

                     if( ! sender( instance.process.queue, common::message::shutdown::Request{}))
                     {
                        log::error << "could not send shutdown to: " << instance.process << std::endl;
                        //
                        // We try to send it later?
                        //
                        state.pending.replies.emplace_back( common::message::shutdown::Request{}, instance.process.queue);
                     }
                     else
                     {
                        log::internal::debug << "sent shutdown message to: " << instance.process << std::endl;
                     }

                  }
                  catch( const state::exception::Missing&)
                  {
                     // we've already removed the instance
                  }
                  catch( const exception::queue::Unavailable&)
                  {
                     // we assume the instance is down
                  }
               }
            }
         }

         namespace update
         {
            void instances( State& state, const state::Server& server)
            {
               if( server.configured_instances > server.instances.size())
               {
                  return boot( state, server, server.configured_instances - server.instances.size());
               }
               else
               {
                  return shutdown( state, server, server.instances.size() - server.configured_instances);
               }
            }
         } // update

         namespace traffic
         {
            void Connect::operator () ( common::message::traffic::monitor::connect::Request& message)
            {
               trace::internal::Scope trace{ "broker::handle::monitor::Connect::dispatch"};

               if( local::handle::connect( m_state, message, common::message::reverse::type( message)))
               {
                  if( ! range::find( m_state.traffic.monitors, message.process.queue))
                  {
                     m_state.traffic.monitors.push_back( message.process.queue);
                  }
                  else
                  {
                     log::error << "traffic monitor already connected - action: ignore" << std::endl;
                  }
               }

            }

            void Disconnect::operator () ( message_type& message)
            {
               trace::internal::Scope trace{ "broker::handle::monitor::Disconnect::dispatch"};

               auto found = range::find( m_state.traffic.monitors, message.process.queue);

               if( found)
               {
                  m_state.traffic.monitors.erase( found.first);
               }
               else
               {
                  log::error << "traffic monitor has already disconnected or not connected in the first place - action: ignore" << std::endl;
               }

            }
         }






         namespace transaction
         {
            namespace manager
            {

               void Connect::operator () ( common::message::transaction::manager::connect::Request& message)
               {
                  common::trace::internal::Scope trace{ "broker::handle::transaction::manager::Connect::dispatch"};

                  common::log::internal::debug << "connect request: " << message.process << std::endl;


                  if( local::handle::connect( m_state, message, common::message::reverse::type( message)))
                  {

                     m_state.transaction_manager = message.process.queue;

                     //
                     // Send configuration to TM
                     //
                     auto configuration = transform::transaction::configuration( m_state);
                     configuration.correlation = message.correlation;

                     queue::blocking::Send tmQueue{ broker::handle::dead::Process{ m_state.ipc()}};
                     tmQueue( message.process.queue, configuration);
                  }
               }

               void Ready::operator () ( message_type& message)
               {
                  common::trace::internal::Scope trace{ "broker::handle::transaction::manager::Ready::dispatch"};

                  common::log::internal::debug << "connect request: " << message.process << std::endl;

                  if( message.success)
                  {

                     //
                     // TM is up and running
                     //
                     auto& instance = m_state.getInstance( message.process.pid);
                     instance.alterState( state::Server::Instance::State::idle);
                  }
                  else
                  {
                     throw common::exception::signal::Terminate{ "transaction manager failed"};
                  }
               }
            }




            namespace client
            {

               void Connect::operator () ( message_type& message)
               {
                  common::trace::internal::Scope trace{ "broker::handle::transaction::client::Connect::dispatch"};

                  common::log::internal::debug << "connect request: " << message.process << std::endl;

                  try
                  {
                     auto& instance = m_state.getInstance( message.process.pid);

                     local::handle::connect( m_state, message, transform::transaction::client::reply( m_state, instance));

                  }
                  catch( const state::exception::Missing& exception)
                  {
                     // What to do? Add the instance?
                     ///log::error << "could not find instance - " << exception.what() << std::endl;

                     common::message::transaction::client::connect::Reply reply;
                     reply.domain = common::environment::domain::name();

                     queue::blocking::Send send( broker::handle::dead::Process{ m_state.ipc()});
                     send( message.process.queue, reply);

                  }
               }
            } // client
         } // transaction


         namespace forward
         {

            void Connect::operator () ( const common::message::forward::connect::Request& message)
            {
               common::trace::internal::Scope trace{ "broker::handle::forward::Connect"};

               if( local::handle::connect( m_state, message, common::message::reverse::type( message)))
               {
                  m_state.forward = message.process;
               }
            }

         } // forward

         namespace dead
         {
            namespace process
            {
               void Registration::operator () ( const common::message::dead::process::Registration& message)
               {
                  common::trace::internal::Scope trace{ "broker::handle::dead::process::Registration"};

                  m_state.dead.process.listeners.push_back( message.process);

                  m_state.dead.process.listeners = range::to_vector( range::unique( range::sort( m_state.dead.process.listeners)));

                  common::log::internal::debug << "dead process listeners: " << common::range::make( m_state.dead.process.listeners) << '\n';

               }


               void Event::operator() ( const common::message::dead::process::Event& event)
               {
                  common::trace::internal::Scope trace{ "broker::handle::dead::process::Event"};

                  if( event.death.deceased())
                  {
                     if( event.death.reason == common::process::lifetime::Exit::Reason::exited)
                     {
                        log::information << "process exited: " << event.death << '\n';
                     }
                     else
                     {
                        log::error << "process terminated: " << event.death << '\n';
                     }

                     //
                     // We remove from listeners if one of them has died.
                     //
                     m_state.dead.process.listeners.erase(
                           range::find_if(
                                 m_state.dead.process.listeners, common::process::Handle::equal::pid{ event.death.pid}).first,
                            std::end( m_state.dead.process.listeners));

                     if( ! m_state.dead.process.listeners.empty())
                     {
                        auto get_queues = [&](){
                           std::vector< platform::queue_id_type> result;
                           for( auto& listener : m_state.dead.process.listeners)
                           {
                              result.push_back( listener.queue);
                           }
                           return result;
                        };

                        common::message::pending::Message message{ event, get_queues()};

                        queue::non_blocking::Send send{ broker::handle::dead::Process{ m_state.ipc()}};
                        auto sender = common::message::pending::sender( send);

                        if( ! sender( message))
                        {
                           m_state.pending.replies.push_back( std::move( message));
                        }
                     }

                     auto& server = m_state.getServer( m_state.getInstance( event.death.pid).server);

                     ++server.deaths;

                     m_state.remove_process( event.death.pid);

                     if( server.restart)
                     {
                        update::instances( m_state, server);
                     }
                  }
                  else
                  {
                     //
                     // TODO: should we warn about this? Don't even know if it's possible for this to happen
                     //
                     log::warning << "process is not in a proper state - " << event.death << '\n';
                  }
               }
            } // process

            Process::Process( common::ipc::receive::Queue& ipc) : m_ipc( ipc) {}

            void Process::operator() ( const common::process::lifetime::Exit& exit)
            {
               //
               // We try to send a event to our self, if it's not possible (queue is full) we put it in pending
               //

               common::message::dead::process::Event event{ exit};

               queue::non_blocking::Send send{ *this};

               common::queue::non_blocking::force::send( m_ipc, send, event);
            }


         } // dead

         namespace lookup
         {
            void Process::operator () ( const common::message::lookup::process::Request& message)
            {
               common::trace::internal::Scope trace{ "broker::handle::lookup::Process"};

               auto reply = common::message::reverse::type( message);
               reply.domain = common::environment::domain::name();
               reply.identification = message.identification;

               auto found = range::find( m_state.singeltons, message.identification);

               auto send_reply = [&](){
                  queue::non_blocking::Send send{ broker::handle::dead::Process{ m_state.ipc()}};
                  if( ! send( message.process.queue, reply))
                  {
                     m_state.pending.replies.emplace_back( reply, message.process.queue);
                  }
               };

               if( found)
               {
                  reply.process = found->second;
                  send_reply();
               }
               else if( message.directive == common::message::lookup::process::Request::Directive::direct)
               {
                  send_reply();
               }
               else
               {
                  m_state.pending.process_lookup.push_back( message);
               }
            }
         }

         void Advertise::operator () ( message_type& message)
         {
            try
            {
               std::vector< state::Service> services;

               common::range::transform( message.services, services, transform::Service{});

               m_state.addServices( message.process.pid, std::move( services));
            }
            catch( ...)
            {
               local::handle::error();
            }

         }

         void Unadvertise::operator () ( message_type& message)
         {
            try
            {
               std::vector< state::Service> services;

               common::range::transform( message.services, services, transform::Service{});

               m_state.removeServices( message.process.pid, std::move( services));
            }
            catch( ...)
            {
               local::handle::error();
            }
         }


         void Connect::operator () ( message_type& message)
         {
            common::trace::internal::Scope trace{ "broker::handle::Connect::dispatch"};

            common::log::internal::debug << "connect request: " << message.process << std::endl;

            try
            {
               //
               // Instance is started for the first time.
               //

               auto& instance = m_state.getInstance( message.process.pid);
               instance.process.queue = message.process.queue;

               if( local::handle::connect( m_state, message, common::message::reverse::type( message)))
               {
                  //
                  // Add services
                  //
                  {
                     std::vector< state::Service> services;

                     common::range::transform( message.services, services, transform::Service{});

                     m_state.addServices( message.process.pid, std::move( services));
                  }

                  //
                  // Set the instance to idle state
                  //
                  instance.alterState( state::Server::Instance::State::idle);
               }

            }
            catch( const state::exception::Missing& exception)
            {
               //
               // The instance was started outside the broker. We dont't allow
               // a server to connect on their own. casual-broker has to start
               // the instances. I think we're better of to keep it simple and
               // strict.
               //

               common::log::error << "process " << message.process << " tried to join the domain on it's own - action: don't allow and send terminate signal" << std::endl;

               common::process::terminate( message.process.pid);
            }
         }


         void ServiceLookup::operator () ( message_type& message)
         {

            try
            {
               auto& service = m_state.getService( message.requested);

               //
               // Try to find an idle instance.
               //
               auto idle = common::range::find_if(
                     service.instances,
                     filter::instance::Idle{});

               //
               // Prepare the message
               //

               auto reply = common::message::reverse::type( message);

               {
                  reply.service = service.information;
                  reply.service.traffic_monitors = m_state.traffic.monitors;

               }


               if( idle)
               {
                  //
                  // flag it as busy.
                  //
                  idle->get().alterState( state::Server::Instance::State::busy);

                  reply.state = decltype( reply.state)::idle;
                  reply.process = transform::Instance()( *idle);

                  queue::blocking::Send send( broker::handle::dead::Process{ m_state.ipc()});
                  send( message.process.queue, reply);

                  service.lookedup++;
               }
               else if(
                  message.context == common::message::service::lookup::Request::Context::no_reply
                  || message.context == common::message::service::lookup::Request::Context::forward)
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

                  queue::blocking::Send send( broker::handle::dead::Process{ m_state.ipc()});
                  send( message.process.queue, reply);

               }
               else
               {
                  //
                  // All instances are busy, we stack the request
                  //
                  m_state.pending.requests.push_back( std::move( message));

                  //
                  // ...and send busy-message to caller, to set timeouts and stuff
                  //
                  reply.state = decltype( reply.state)::busy;

                  queue::blocking::Send send( broker::handle::dead::Process{ m_state.ipc()});
                  send( message.process.queue, reply);

               }

            }
            catch( state::exception::Missing& exception)
            {
               //
               // TODO: We will send the request to the gateway. (only if we want auto discovery)
               //
               // Server (queue) that hosts the requested service is not found.
               // We propagate this by having 0 occurrence of server in the response
               //
               auto reply = common::message::reverse::type( message);
               reply.service.name = message.requested;
               reply.state = decltype( reply.state)::absent;

               queue::blocking::Send send( broker::handle::dead::Process{ m_state.ipc()});
               send( message.process.queue, reply);
            }
         }


         void ACK::operator () ( message_type& message)
         {
            try
            {
               auto& instance = m_state.getInstance( message.process.pid);

               instance.alterState( state::Server::Instance::State::idle);
               ++instance.invoked;

               //
               // Check if there are pending request for services that this
               // instance has.
               //
               {
                  auto pending = common::range::find_if(
                     m_state.pending.requests,
                     filter::Pending( instance));

                  if( pending)
                  {

                     //
                     // We now know that there are one idle server that has advertised the
                     // requested service (we've just marked it as idle...).
                     // We can use the normal request to get the response
                     //
                     ServiceLookup lookup( m_state);
                     lookup( *pending);

                     //
                     // Remove pending
                     //
                     m_state.pending.requests.erase( pending.first);
                  }

               }
            }
            catch( state::exception::Missing& exception)
            {
               common::log::error << "failed to find instance on ACK - indicate inconsistency " << exception.what() <<std::endl;
            }
         }


         void Policy::connect( common::ipc::receive::Queue& ipc, std::vector< common::message::Service> services, const std::vector< common::transaction::Resource>& resources)
         {
            m_state.connect_broker( std::move( services));
         }


         void Policy::reply( platform::queue_id_type id, common::message::service::call::Reply& message)
         {
            queue::blocking::Send send( broker::handle::dead::Process{ m_state.ipc()});
            send( id, message);
         }

         void Policy::ack( const common::message::service::call::callee::Request& message)
         {
            common::message::service::call::ACK ack;

            ack.process = common::process::handle();
            ack.service = message.service.name;

            ACK sendACK( m_state);
            sendACK( ack);
         }

         void Policy::transaction( const common::message::service::call::callee::Request&, const server::Service&, const common::platform::time_point&)
         {
            // broker doesn't bother with transactions...
         }

         void Policy::transaction( const common::message::service::call::Reply& message, int return_state)
         {
            // broker doesn't bother with transactions...
         }

         void Policy::forward( const common::message::service::call::callee::Request& message, const common::server::State::jump_t& jump)
         {
            throw common::exception::xatmi::System{ "can't forward within broker"};
         }

         void Policy::statistics( platform::queue_id_type id,common::message::traffic::Event&)
         {
            //
            // We don't collect statistics for the broker
            //
         }

      } // handle

      common::message::dispatch::Handler handler( State& state)
      {
         return {
            handle::transaction::manager::Connect{ state},
            handle::transaction::manager::Ready{ state},
            handle::forward::Connect{ state},
            handle::dead::process::Registration{ state},
            handle::dead::process::Event{ state},
            handle::lookup::Process{ state},
            handle::Connect{ state},
            handle::Advertise{ state},
            handle::Unadvertise{ state},
            handle::ServiceLookup{ state},
            handle::ACK{ state},
            handle::traffic::Connect{ state},
            handle::traffic::Disconnect{ state},
            handle::transaction::client::Connect{ state},
            handle::Call{ ipc::receive::queue(), admin::services( state), state},
            common::message::handle::ping( broker::handle::dead::Process{ state.ipc()}),
            common::message::handle::Shutdown{},
         };
      }

      common::message::dispatch::Handler handler_no_services( State& state)
      {
         return {
            handle::transaction::manager::Connect{ state},
            handle::transaction::manager::Ready{ state},
            handle::forward::Connect{ state},
            handle::dead::process::Registration{ state},
            handle::dead::process::Event{ state},
            handle::lookup::Process{ state},
            handle::Connect{ state},
            handle::Advertise{ state},
            handle::Unadvertise{ state},
            handle::ServiceLookup{ state},
            handle::ACK{ state},
            handle::traffic::Connect{ state},
            handle::traffic::Disconnect{ state},
            handle::transaction::client::Connect{ state},
            //handle::Call{ ipc::receive::queue(), admin::services( state), state},
            common::message::handle::ping( broker::handle::dead::Process{ state.ipc()}),
            common::message::handle::Shutdown{},
         };
      }
   } // broker
} // casual
