//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "queue/manager/manager.h"
#include "queue/manager/admin/server.h"
#include "queue/manager/handle.h"

#include "queue/common/log.h"


#include "common/message/dispatch.h"
#include "common/message/handle.h"
#include "common/algorithm.h"
#include "common/process.h"
#include "common/environment.h"
#include "common/environment/normalize.h"
#include "common/exception/signal.h"
#include "common/exception/handle.h"
#include "common/event/send.h"

#include "common/communication/instance.h"

#include "configuration/message/transform.h"
#include "configuration/queue.h"

#include <fstream>


namespace casual
{
   using namespace common;
   namespace queue
   {
      namespace manager
      {

         namespace local
         {
            namespace
            {

               namespace transform
               {

                  State state( const Settings& settings)
                  {
                     Trace trace( "queue::manager::local::transform::state");

                     State result;

                     result.group_executable = common::coalesce(
                        std::move( settings.group.executable),
                        casual::common::environment::directory::casual() + "/bin/casual-queue-group"
                     );

                     // We ask the domain manager for configuration
                     result.configuration = common::communication::ipc::call(
                        common::communication::instance::outbound::domain::manager::device(), 
                        common::message::domain::configuration::Request{ common::process::handle()}).domain.queue;

                     common::environment::normalize( result.configuration);

                     return result;
                  }
               } // transform


               struct Spawn : manager::handle::Base
               {
                  using manager::handle::Base::Base;

                  State::Group operator () ( const common::message::domain::configuration::queue::Group& group)
                  {
                     Trace trace( "queue::manager::local::Spawn");

                     State::Group result;
                     result.name = group.name;
                     result.queuebase = group.queuebase;

                     try
                     {
                        result.process.pid = casual::common::process::spawn(
                           m_state.group_executable,
                           { "--queuebase", group.queuebase, "--name", group.name});
                     }
                     catch( const common::exception::base& exception)
                     {
                        auto message = common::string::compose( "failed to spawn queue group:  ", group.name, " - ", exception);
                        
                        common::log::line( common::log::category::error, message);
                        common::event::error::send( message, common::event::error::Severity::error);
                     }

                     return result;
                  }
               };



               void startup( State& state)
               {
                  Trace trace( "queue::manager::local::startup");
                  
                  casual::common::algorithm::transform( state.configuration.groups, state.groups, Spawn( state));

                  common::algorithm::trim( state.groups, common::algorithm::remove_if( state.groups, []( auto& g){
                     return ! g.process.pid;
                  }));
            
                  // Make sure all groups are up and running before we continue
                  {
                     auto handler = ipc::device().handler(
                        manager::handle::connect::Request{ state},
                        manager::handle::connect::Information{ state},
                        manager::handle::process::Exit{ state},
                        common::message::handle::Shutdown{}
                     );

                     const auto filter = handler.types();

                     // TODO maintainence: change to message::dispatch::...conditional
                     while( ! common::algorithm::all_of( state.groups, std::mem_fn(&State::Group::connected)))
                        handler( communication::ipc::blocking::next( ipc::device(), filter));
                  }
               }

            } // <unnamed>
         } // local
      } // manager


      Manager::Manager( manager::Settings settings) 
         : m_state{ manager::local::transform::state( settings)}
      {
         Trace trace( "queue::Manager::Manager");

         // make sure we handle death of our children
         signal::callback::registration< code::signal::child>( []()
         {
            algorithm::for_each( process::lifetime::ended(), []( auto& exit)
            {
               manager::handle::process::exit( exit);
            });
         }); 

         // Set environment variable so children can find us easy
         common::environment::variable::process::set(
               common::environment::variable::name::ipc::queue::manager(),
               common::process::handle());
      }

      Manager::~Manager()
      {
         Trace trace( "queue::Manager::~Manager");

         try
         {
            common::process::children::terminate(
               [&]( auto& exit)
               {
                  manager::handle::process::Exit{ m_state}( message::event::process::Exit{ exit});
               },
               m_state.processes());

            common::log::line( common::log::category::information, "casual-queue-manager is off-line");

         }
         catch( const common::exception::signal::Timeout& exception)
         {
            auto pids = m_state.processes();
            common::log::line( common::log::category::error, "failed to terminate groups - pids: ", pids);
         }
         catch( ...)
         {
            common::exception::handle();
         }

      }

      void Manager::start()
      {
         common::log::line( log, "queue manager start");

         manager::local::startup( m_state);

         auto handler = manager::handlers( m_state);

         // Connect to domain
         common::communication::instance::connect( common::communication::instance::identity::queue::manager);

         common::log::line( common::log::category::information, "casual-queue-manager is on-line");
         
         message::dispatch::blocking::pump( handler, manager::ipc::device());
      }
   } // queue

} // casual
