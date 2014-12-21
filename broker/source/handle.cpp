//!
//! handle.cpp
//!
//! Created on: Jun 1, 2014
//!     Author: Lazan
//!

#include "broker/handle.h"
#include "broker/transform.h"
#include "broker/filter.h"

#include "common/server/lifetime.h"
#include "common/queue.h"
#include "common/internal/log.h"
#include "common/environment.h"
#include "common/algorithm.h"
#include "common/process.h"
#include "common/message/dispatch.h"


//
// std
//
#include <vector>
#include <string>

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


                  void operator () ( const state::Executable& server)
                  {
                     if( server.configuredInstances > server.instances.size())
                     {
                        decltype( server.instances) pids;

                        auto count = server.configuredInstances - server.instances.size();

                        while( count-- > 0)
                        {
                           pids.push_back( common::process::spawn( server.path, server.arguments));
                        }

                        m_state.addInstances( server.id, std::move( pids));
                     }
                  }


                  void operator () ( State::Batch& batch)
                  {
                     common::log::information << "boot group '" << batch.group << "'\n";

                     common::range::for_each( batch.servers, *this);
                     common::range::for_each( batch.executables, *this);


                     queue::blocking::Reader queueReader{ common::ipc::receive::queue(), m_state};

                     common::message::dispatch::Handler handler{
                        handle::transaction::manager::Connect{ m_state},
                        handle::transaction::manager::Ready{ m_state},
                        handle::Connect{ m_state},
                        handle::transaction::client::Connect{ m_state},
                     };

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
                           handler.dispatch( marshal);
                        }
                        catch( const common::exception::signal::Timeout& exception)
                        {
                           common::log::error << "failed to get response from spawned instances in a timely manner - action: abort" << std::endl;
                           throw common::exception::signal::Terminate{};
                        }
                     }
                  }
               };

               struct Shutdown : state::Base
               {
                  using state::Base::Base;


                  void operator () ( State::Batch& batch)
                  {
                     common::log::information << "shutdown group '" << batch.group << "'\n";

                     //
                     // Take care of executables
                     //

                     std::vector< platform::pid_type> executables;

                     for( auto& executable : batch.executables)
                     {
                        range::append( executable.get().instances, executables);
                     }


                     //
                     // Take care of xatmi-servers
                     //

                     std::vector< process::Handle> servers;

                     for( auto& server : batch.servers)
                     {
                        for( auto& pid : server.get().instances)
                        {
                           //
                           // Make sure we don't add our self
                           //
                           if( pid != process::id())
                           {
                              servers.push_back( m_state.getInstance( pid).process);
                           }
                        }
                     }
                     server::lifetime::shutdown( m_state, servers, executables, std::chrono::seconds( 2));
                  }
               };


            } // <unnamed>
         } // local

         void boot( State& state)
         {
            auto bootOrder = state.bootOrder();

            range::for_each( bootOrder, local::Boot{ state});
         }


         void shutdown( State& state)
         {
            auto shutdownOrder = state.bootOrder();
            std::reverse( std::begin( shutdownOrder), std::end( shutdownOrder));

            try
            {
               signal::timer::Scoped alarm( std::chrono::seconds( 10));

               range::for_each( shutdownOrder, local::Shutdown{ state});
            }
            catch( const exception::signal::Timeout&)
            {
               log::error << "failed to shutdown - TODO: send reply" << std::endl;
            }
         }

         void MonitorConnect::dispatch( message_type& message)
         {
            //TODO: Temp
            m_state.monitorQueue = message.process.queue;
         }

         void MonitorDisconnect::dispatch( message_type& message)
         {
            m_state.monitorQueue = 0;
         }


         namespace transaction
         {
            namespace manager
            {

               void Connect::dispatch( message_type& message)
               {
                  common::trace::internal::Scope trace{ "broker::handle::transaction::manager::Connect::dispatch"};

                  common::log::internal::debug << "connect request: " << message.process << std::endl;

                  m_state.transactionManagerQueue = message.process.queue;

                  //
                  // Send configuration to TM
                  //
                  auto configuration = transform::transaction::configuration( m_state);

                  queue::blocking::Writer tmQueue{ message.process.queue, m_state};
                  tmQueue( configuration);

               }

               void Ready::dispatch( message_type& message)
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

               void Connect::dispatch( message_type& message)
               {
                  common::trace::internal::Scope trace{ "broker::handle::transaction::client::Connect::dispatch"};

                  common::log::internal::debug << "connect request: " << message.process << std::endl;

                  try
                  {
                     auto& instance = m_state.getInstance( message.process.pid);


                     //
                     // Instance is started for the first time.
                     // Send some configuration
                     //
                     message::transaction::client::connect::Reply reply =
                           transform::transaction::client::reply( m_state, instance);

                     queue::blocking::Writer write( message.process.queue, m_state);
                     write( reply);
                  }
                  catch( const state::exception::Missing& exception)
                  {
                     // What to do? Add the instance?
                     ///log::error << "could not find instance - " << exception.what() << std::endl;

                     message::transaction::client::connect::Reply reply;
                     reply.domain = common::environment::domain::name();
                     reply.transactionManagerQueue = m_state.transactionManagerQueue;

                     queue::blocking::Writer write( message.process.queue, m_state);
                     write( reply);

                  }
               }
            } // client
         } // transaction


         void Advertise::dispatch( message_type& message)
         {

            std::vector< state::Service> services;

            common::range::transform( message.services, services, transform::Service{});

            m_state.addServices( message.process.pid, std::move( services));

         }

         void Unadvertise::dispatch( message_type& message)
         {
            std::vector< state::Service> services;

            common::range::transform( message.services, services, transform::Service{});

            m_state.removeServices( message.process.pid, std::move( services));
         }


         void Connect::dispatch( message_type& message)
         {
            common::trace::internal::Scope trace{ "broker::handle::Connect::dispatch"};

            common::log::internal::debug << "connect request: " << message.process << std::endl;

            try
            {

               auto& instance = m_state.getInstance( message.process.pid);
               instance.process.queue = message.process.queue;

               //
               // Instance is started for the first time.
               // Send some configuration
               //
               message::server::connect::Reply reply;

               common::log::internal::debug << "connect reply: " << message.process << std::endl;

               queue::blocking::Writer writer( message.process.queue, m_state);
               writer( reply);


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
            catch( const state::exception::Missing& exception)
            {
               //
               // The instance was started outside the broker. We dont't allow
               // a server to connect on their own. casual-broker has to start
               // the instances. I think we're better of to keep it simple and
               // strict.
               //

               common::log::error << "process " << message.process << " tried to join the domain on it's own - acion: don't allow and send terminate signal" << std::endl;

               common::process::terminate( message.process.pid);
            }
         }


         void ServiceLookup::dispatch( message_type& message)
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


               if( idle)
               {
                  //
                  // flag it as busy.
                  //
                  idle->get().alterState( state::Server::Instance::State::busy);

                  message::service::name::lookup::Reply reply;
                  reply.service = service.information;
                  reply.service.monitor_queue = m_state.monitorQueue;
                  reply.supplier = transform::Instance()( *idle);

                  queue::blocking::Writer writer( message.process.queue, m_state);
                  writer( reply);

                  service.lookedup++;
               }
               else
               {
                  //
                  // All servers are busy, we stack the request
                  //
                  m_state.pending.push_back( std::move( message));
               }

            }
            catch( state::exception::Missing& exception)
            {
               //
               // TODO: We will send the request to the gateway.
               //
               // Server (queue) that hosts the requested service is not found.
               // We propagate this by having 0 occurrence of server in the response
               //
               message::service::name::lookup::Reply reply;
               reply.service.name = message.requested;

               queue::blocking::Writer writer( message.process.queue, m_state);
               writer( reply);
            }
         }


         void ACK::dispatch( message_type& message)
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
                     common::range::make( m_state.pending),
                     filter::Pending( instance));

                  if( pending)
                  {

                     //
                     // We now know that there are one idle server that has advertised the
                     // requested service (we've just marked it as idle...).
                     // We can use the normal request to get the response
                     //
                     ServiceLookup lookup( m_state);
                     lookup.dispatch( *pending);

                     //
                     // Remove pending
                     //
                     m_state.pending.erase( pending.first);
                  }

               }
            }
            catch( state::exception::Missing& exception)
            {
               common::log::error << "failed to find instance on ACK - indicate inconsistency " << exception.what() <<std::endl;
            }
         }


         void Policy::connect( std::vector< common::message::Service> services, const std::vector< common::transaction::Resource>& resources)
         {

            //
            // We add the server
            // TODO:
            auto& server = [&]() -> state::Server& {
               state::Server server;
               server.alias = "casual-broker";
               server.configuredInstances = 1;
               server.path = common::process::path();
               server.instances.push_back( common::process::id());
               return m_state.add( std::move( server));
            }();


            {
               state::Server::Instance instance;
               instance.process = common::process::handle();
               instance.server = server.id;
               instance.alterState( state::Server::Instance::State::idle);

               m_state.add( std::move( instance));
            }

            //
            // Add services
            //
            {
               std::vector< state::Service> brokerServices;

               common::range::transform( services, brokerServices, transform::Service{});

               m_state.addServices( common::process::id(), std::move( brokerServices));
            }
         }


         void Policy::reply( platform::queue_id_type id, message::service::Reply& message)
         {
            queue::blocking::Writer writer( id, m_state);
            writer( message);
         }

         void Policy::ack( const message::service::callee::Call& message)
         {
            message::service::ACK ack;

            ack.process = common::process::handle();
            ack.service = message.service.name;

            ACK sendACK( m_state);
            sendACK.dispatch( ack);
         }

         void Policy::transaction( const message::service::callee::Call&, const server::Service&, const common::platform::time_point&)
         {
            // broker doesn't bother with transactions...
         }

         void Policy::transaction( const message::service::Reply& message)
         {
            // broker doesn't bother with transactions...
         }

         void Policy::statistics( platform::queue_id_type id, message::monitor::Notify& message)
         {
            //
            // We don't collect statistics for the broker
            //
         }



      } // handle
   } // broker
} // casual
