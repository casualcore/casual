//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "gateway/manager/handle.h"
#include "gateway/manager/admin/server.h"
#include "gateway/environment.h"
#include "gateway/common.h"

#include "common/server/handle/call.h"
#include "common/message/handle.h"
#include "common/message/internal.h"
#include "common/communication/instance.h"

#include "common/event/send.h"
#include "common/environment.h"
#include "common/process.h"
#include "common/cast.h"

namespace casual
{
   using namespace common;

   namespace gateway::manager
   {
      namespace ipc
      {
         common::communication::ipc::inbound::Device& inbound()
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
               namespace shutdown
               {
                  auto group()
                  {
                     return []( auto& group)
                     {
                        Trace trace{ "gateway::manager::handle::local::shutdown::group"};

                        // We only want to handle terminate during this
                        common::signal::thread::scope::Mask mask{ signal::set::filled( code::signal::terminate)};

                        if( group.process)
                        {
                           log::line( log, "send shutdown to 'group': ", group);

                           communication::device::blocking::optional::send( 
                              group.process.ipc, 
                              common::message::shutdown::Request{ common::process::handle()});
                        }
                        else if( group.process.pid)
                        {
                           log::line( log, "terminate group: ", group);
                           signal::send( group.process.pid, code::signal::terminate);
                        }
                     };
                  }

               } // shutdown


               namespace spawn
               {
                  strong::process::id process( const std::string& alias, const std::string& path, const std::vector< std::string>& arguments)
                  {
                     try
                     {                          
                        auto pid = common::process::spawn(
                           path,
                           arguments);

                        // Send event
                        {
                           common::message::event::process::Spawn event{ common::process::handle()};
                           event.path = path;
                           event.alias = alias;
                           event.pids.push_back( pid);
                           common::event::send( event);
                        }

                        return pid;
                        
                     }
                     catch( ...)
                     {
                        log::line( log::category::error, exception::capture(), " spawn process ", path);
                     }
                     return {};
                  }
   
                  auto group()
                  {
                     return []( auto& group)
                     {
                        group.process.pid = spawn::process( group.configuration.alias, state::executable::path( group),
                           {});
                     };
                  }

               } // spawn

            } // <unnamed>
         } // local

         void shutdown( State& state)
         {
            Trace trace{ "gateway::manager::handle::shutdown"};

            state.runlevel = state::Runlevel::shutdown;

            log::line( verbose::log, "state: ", state);

            algorithm::for_each( state.inbound.groups, local::shutdown::group());
            algorithm::for_each( state.outbound.groups, local::shutdown::group());
         }

         void boot( State& state)
         {
            Trace trace{ "gateway::manager::handle::boot"};

            auto boot = [handler = manager::handler( state)]( auto& groups) mutable
            {
               algorithm::for_each( groups, local::spawn::group());

               common::message::dispatch::relaxed::pump(
                  common::message::dispatch::condition::compose(
                     common::message::dispatch::condition::done( [&groups]()
                     {
                        auto connected = []( auto& bound)
                        {
                           return predicate::boolean( bound.process);
                        };

                        return algorithm::all_of( groups, connected);
                     })
                  ),
                  handler,
                  ipc::inbound());
            };

            // make sure we boot 'in order' and got connected stuff before we continue
            boot( state.outbound.groups);
            boot( state.inbound.groups);
            
         }

         namespace process
         {
            void exit( const common::process::lifetime::Exit& exit)
            {
               Trace trace{ "gateway::manager::handle::process::exit"};

               common::message::event::process::Exit event{ exit};

               // Send the exit notification to domain.
               communication::device::blocking::send( communication::instance::outbound::domain::manager::device(), event);

               // We put a dead process event on our own ipc device, that
               // will be handled later on.
               communication::ipc::inbound::device().push( std::move( event));
            }

         } // process

      

         namespace local
         {
            namespace
            {
               namespace process
               {
                  auto exit( State& state)
                  {  
                     return [&state]( const common::message::event::process::Exit& message)
                     {
                        Trace trace{ "gateway::manager::handle::local::process::exit"};

                        if( ! message.state.deceased())
                           return;

                        const auto pid = message.state.pid;

                        auto restart = [&state, &message]( auto& group, auto information)
                        {
                           if( state.runlevel == decltype( state.runlevel())::running)
                           {
                              log::line( log::category::error, code::casual::invalid_semantics, ' ', information,  
                                 message.state.reason, " - pid: ", message.state.pid, " alias: ", group.configuration.alias, " - action: restart");

                              group.process = {};
                              local::spawn::group()( group);
                              return true;
                           }
                           return  false;
                        };

                        if( auto found = algorithm::find( state.inbound.groups, pid))
                        {
                           log::line( log::category::information, "inbound terminated - alias: ", found->configuration.alias);
                           log::line( verbose::log, "connection: ", *found);

                           if( ! restart( *found, "inbound"))
                              state.inbound.groups.erase( std::begin( found));
                        }
                        
                        if( auto found = algorithm::find( state.outbound.groups, pid))
                        {
                           log::line( log::category::information, "outbound terminated - alias: ", found->configuration.alias);
                           log::line( verbose::log, "connection: ", *found);

                           if( ! restart( *found, "outbound"))
                              state.outbound.groups.erase( std::begin( found));
                        }
                     };
                  }

               } // process

               namespace outbound
               {
                  auto connect( State& state)
                  {
                     return [&state]( message::outbound::Connect& message)
                     {
                        Trace trace{ "gateway::manager::handle::local::outbound::connect"};
                        log::line( verbose::log, "message: ", message);

                        if( auto found = algorithm::find( state.outbound.groups, message.process.pid))
                        {
                           found->process = message.process;

                           message::outbound::configuration::update::Request request{ common::process::handle()};
                           request.model = found->configuration;
                           communication::device::blocking::optional::send( message.process.ipc, request);                                 
                        }
                        else
                           log::line( log::category::error, code::casual::internal_correlation, " failed to correlate reverse outbound pid ", message.process.pid, " - action: ignore");
                     };
                  }

                  namespace configuration::update
                  {
                     auto reply( State& state)
                     {
                        return []( message::outbound::configuration::update::Reply& message)
                        {
                           Trace trace{ "gateway::manager::handle::local::outbound::configuration::update"};
                           log::line( verbose::log, "message: ", message);

                           log::line( log::category::information, "outbound configured - pid: ", message.process.pid);
                        };
                     }
                     
                  } // configuration::update
               } // outbound

               namespace inbound
               {
                  auto connect( State& state)
                  {
                     return [&state]( message::inbound::Connect& message)
                     {
                        Trace trace{ "gateway::manager::handle::local::inbound::connect"};
                        log::line( verbose::log, "message: ", message);

                        if( auto found = algorithm::find( state.inbound.groups, message.process.pid))
                        {
                           found->process = message.process;

                           message::inbound::configuration::update::Request request{ common::process::handle()};
                           request.model = found->configuration;
                           communication::device::blocking::optional::send( message.process.ipc, request);                                 
                        }
                        else
                           log::line( log::category::error, code::casual::internal_correlation, " failed to correlate inbound pid ", message.process.pid, " - action: ignore");
                     };
                  }


                  namespace configuration::update
                  {
                     auto reply( State& state)
                     {
                        return []( message::inbound::configuration::update::Reply& message)
                        {
                           Trace trace{ "gateway::manager::handle::local::inbound::configuration::update"};
                           log::line( verbose::log, "message: ", message);

                           log::line( log::category::information, "inbound configured - pid: ", message.process.pid);
                        };
                     }
                     
                  } // configuration::update

               } // inbound

               namespace shutdown
               {
                  auto request( State& state)
                  {
                     return [&state]( common::message::shutdown::Request& message)
                     {
                        Trace trace{ "gateway::manager::handle::local::shutdown::request"};
                        log::line( verbose::log, "message: ", message);

                        handle::shutdown( state);
                     };
                  }
               } // shutdown


            } // <unnamed>
         } // local
      } // handle

      handle::dispatch_type handler( State& state)
      {
         static common::server::handle::admin::Call call{ manager::admin::services( state)};

         return common::message::dispatch::handler( ipc::inbound(),
            common::message::handle::defaults( ipc::inbound()),
            common::message::internal::dump::state::handle( state),
            handle::local::process::exit( state),

            handle::local::outbound::connect( state),
            handle::local::outbound::configuration::update::reply( state),
            handle::local::inbound::connect( state),
            handle::local::inbound::configuration::update::reply( state),

            handle::local::shutdown::request( state),

            std::ref( call));
      }

   } // gateway::manager
} // casual
