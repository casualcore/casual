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


               auto spawn( State& state)
               {
                  return [&state]( const common::message::domain::configuration::queue::Group& group)
                  {
                     Trace trace( "queue::manager::local::Spawn");

                     State::Group result;
                     result.name = group.name;
                     result.queuebase = group.queuebase;

                     try
                     {
                        result.process.pid = casual::common::process::spawn(
                           state.group_executable,
                           { "--queuebase", group.queuebase, "--name", group.name});
                     }
                     catch( ...)
                     {
                        common::event::error::send( exception::code(), "failed to spawn queue group:  ", group.name);
                     }

                     return result;
                  };
               }


               void startup( State& state)
               {
                  Trace trace( "queue::manager::local::startup");
                  
                  casual::common::algorithm::transform( state.configuration.groups, state.groups, local::spawn( state));

                  common::algorithm::trim( state.groups, common::algorithm::remove_if( state.groups, []( auto& g){
                     return ! g.process.pid;
                  }));

                  namespace dispatch = common::message::dispatch;

                  auto condition = dispatch::condition::compose( 
                     dispatch::condition::done( [&]()
                     {
                        // Make sure all groups are up and running before we continue
                        return common::algorithm::all_of( state.groups, []( auto& group){ return group.connected();});
                     })
                  );
            
                  dispatch::pump( condition,
                     manager::startup::handlers( state),
                     ipc::device());

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
               common::environment::variable::name::ipc::queue::manager,
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
                  m_state.remove( exit.pid);
               },
               m_state.processes());

            common::log::line( common::log::category::information, "casual-queue-manager is off-line");

         }
         catch( ...)
         {
            common::log::line( 
               common::log::category::error, 
               common::exception::code(), 
               " termination of groups failed - pids: ",  m_state.processes());
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
         
         message::dispatch::pump( handler, manager::ipc::device());
      }
   } // queue

} // casual
