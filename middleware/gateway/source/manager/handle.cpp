//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "gateway/manager/handle.h"
#include "gateway/manager/admin/server.h"
#include "gateway/manager/transform.h"
#include "gateway/manager/configuration.h"
#include "gateway/environment.h"
#include "gateway/common.h"

#include "configuration/message.h"

#include "common/server/handle/call.h"
#include "common/message/dispatch/handle.h"
#include "common/message/internal.h"
#include "common/instance.h"
#include "common/communication/instance.h"

#include "common/event/send.h"
#include "common/environment.h"
#include "common/process.h"

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
         void shutdown( State& state)
         {
            Trace trace{ "gateway::manager::handle::shutdown"};

            state.runlevel = state::Runlevel::shutdown;

            // conform to empty configuration
            configuration::conform( state, {});

            log::line( verbose::log, "state: ", state);
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
                     return [ &state]( const common::message::event::process::Exit& message)
                     {
                        Trace trace{ "gateway::manager::handle::local::process::exit"};
                        log::line( verbose::log, "state.runlevel: ", state.runlevel);

                        if( ! message.state.deceased())
                           return;

                        const auto pid = message.state.pid;

                        auto do_restart = [ &state, reason = message.state.reason]( auto& group, auto description)
                        {
                           if( state.runlevel == decltype( state.runlevel())::running)
                           {
                              event::error::send( code::casual::invalid_semantics, description, " group ", group.configuration.alias, " exited - reason: ",
                                 reason, " - action: restart");
                              return true;
                           }
                           return  false;
                        };

                        if( auto found = algorithm::find( state.inbound.groups, pid))
                        {
                           auto group = algorithm::container::extract( state.inbound.groups, std::begin( found));
                           if( do_restart( group, "inbound"))
                              manager::configuration::add::group( state, std::move( group.configuration));
                        }

                        if( auto found = algorithm::find( state.outbound.groups, pid))
                        {
                           auto group = algorithm::container::extract( state.outbound.groups, std::begin( found));
                           if( do_restart( group, "outbound"))
                              manager::configuration::add::group( state, std::move( group.configuration));
                        }

                        // dispatch to potential tasks.
                        state.tasks( message);
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
                        return [ &state]( message::outbound::configuration::update::Reply& message)
                        {
                           Trace trace{ "gateway::manager::handle::local::outbound::configuration::update"};
                           log::line( verbose::log, "message: ", message);

                           state.tasks( message);
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
                        return [ &state]( message::inbound::configuration::update::Reply& message)
                        {
                           Trace trace{ "gateway::manager::handle::local::inbound::configuration::update"};
                           log::line( verbose::log, "message: ", message);

                           state.tasks( message);
                        };
                     }
                     
                  } // configuration::update

               } // inbound

               namespace configuration
               {
                  auto request( const State& state)
                  {
                     return [&state]( casual::configuration::message::Request& message)
                     {
                        Trace trace{ "gateway::manager::handle::local::configuration::request"};
                        common::log::line( verbose::log, "message: ", message);

                        auto reply = common::message::reverse::type( message);

                        reply.model.gateway = transform::configuration( state);

                        communication::device::blocking::optional::send( message.process.ipc, reply);
                     };
                  }

                  namespace update
                  {
                     auto request( State& state)
                     {
                        return [ &state]( casual::configuration::message::update::Request& message)
                        {
                           Trace trace{ "gateway::manager::handle::local::configuration::update::request"};
                           log::line( verbose::log, "message: ", message);

                           if( state.runlevel == state::Runlevel::running)
                              state.runlevel.explict_set( state::Runlevel::configuring);

                           manager::configuration::conform( state, std::move( message.model));

                           auto configure_done = [ &state, message]( task::unit::id)
                           {
                              state.runlevel = state::Runlevel::running;

                              communication::device::blocking::optional::send( message.process.ipc, common::message::reverse::type( message));
                              return task::unit::action::Outcome::abort;
                           };

                           state.tasks.then( task::create::unit( std::move( configure_done)));
                        };
                     }
                  } // update
               } // configuration

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
            common::message::dispatch::handle::defaults( state),
            handle::local::process::exit( state),

            handle::local::outbound::connect( state),
            handle::local::outbound::configuration::update::reply( state),
            handle::local::inbound::connect( state),
            handle::local::inbound::configuration::update::reply( state),
            
            handle::local::configuration::request( state),
            handle::local::configuration::update::request( state),
            handle::local::shutdown::request( state),

            std::ref( call));
      }

   } // gateway::manager
} // casual
