//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include "queue/manager/manager.h"
#include "queue/manager/admin/server.h"
#include "queue/manager/handle.h"
#include "queue/common/log.h"
#include "queue/common/ipc/message.h"


#include "common/message/dispatch.h"
#include "common/message/handle.h"
#include "common/algorithm.h"
#include "common/process.h"
#include "common/environment.h"
#include "common/environment/normalize.h"
#include "common/event/send.h"

#include "common/communication/instance.h"

#include "domain/configuration/fetch.h"

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
                  State state( Settings&& settings)
                  {
                     Trace trace( "queue::manager::local::transform::state");

                     State result;

                     result.executable.path = common::coalesce(
                        std::move( settings.executable.path),
                        casual::common::environment::directory::casual() + "/bin");

                     // We ask the domain manager for configuration
                     result.model = casual::domain::configuration::fetch().queue;

                     log::line( verbose::log, "configuration: ", result.model);

                     return result;
                  }
               } // transform

               namespace spawn
               {
                  auto group( const std::string& path)
                  {
                     return [&path]( const auto& group)
                     {
                        Trace trace( "queue::manager::local::spawn::group");

                        State::Group result;
                        result.name = group.name;
                        result.queuebase = group.queuebase;

                        try
                        {
                           result.process.pid = casual::common::process::spawn(
                              path + "/casual-queue-group",
                              { "--queuebase", group.queuebase, "--name", group.name});
                        }
                        catch( ...)
                        {
                           common::event::error::raise( exception::code(), "failed to spawn queue group:  ", group.name);
                        }

                        return result;
                     };
                  }

                  void forwards( State& state)
                  {
                     Trace trace( "queue::manager::local::spawn::forwards");

                     if( ! state.model.forward.services.empty() || ! state.model.forward.queues.empty())
                     {
                        try
                        {
                           state.forwards.emplace_back( casual::common::process::spawn( 
                              string::compose( state.executable.path, '/', "casual-queue-forward"), {}));
                        }
                        catch( ...)
                        {
                           common::event::error::raise( exception::code(), "failed to spawn casual-queue-forward");
                        }
                     }
                  }

               } // spawn


               auto startup( State& state)
               {
                  Trace trace( "queue::manager::local::startup");

                  auto handlers = manager::startup::handlers( state);

                  //! some helpers
                  //! @{
                  auto wait_for_done = [&handlers]( auto&& is_done)
                  {
                     namespace dispatch = common::message::dispatch;

                     auto condition = dispatch::condition::compose( 
                        dispatch::condition::done( [&]()
                        {
                           return is_done();
                        })
                     );
               
                     dispatch::relaxed::pump( condition,
                        handlers,
                        ipc::device());
                  };

                  auto valid_pid = []( auto& value ){ return ! value.process.pid.empty();};
                  //! @}
                  

                  casual::common::algorithm::transform( state.model.groups, state.groups, local::spawn::group( state.executable.path));

                  // only keep spawned groups
                  common::algorithm::trim( state.groups, common::algorithm::filter( state.groups, valid_pid));


                  // Make sure all groups are up and running before we continue
                  wait_for_done( [&state]()
                  {
                     return common::algorithm::all_of( state.groups, []( auto& group){ return group.connected();});
                  });

                  
                  spawn::forwards( state);

                  // Make sure all forwards are up and running before we continue
                  wait_for_done( [&state]()
                  {
                     return common::algorithm::all_of( state.forwards, []( auto& process){ return static_cast< bool>( process);});
                  });

                  return handlers;

               }

            } // <unnamed>
         } // local
      } // manager


      Manager::Manager( manager::Settings settings) 
         : m_state{ manager::local::transform::state( std::move( settings))}
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

         exception::guard( [&]()
         {
            // first we shutdown all forwards
            common::process::lifetime::terminate( m_state.forwards);

            // then we can terminate the groups
            common::process::children::terminate(
               [&]( auto& exit)
               {
                  m_state.remove( exit.pid);
               },
               m_state.processes());

            common::log::line( common::log::category::information, "casual-queue-manager is off-line");
         });

      }

      void Manager::start()
      {
         common::log::line( log, "queue manager start");

         auto handler = manager::handlers( m_state, manager::local::startup( m_state));

         // Connect to domain
         common::communication::instance::connect( common::communication::instance::identity::queue::manager);

         common::log::line( common::log::category::information, "casual-queue-manager is on-line");
         
         message::dispatch::pump( handler, ipc::device());
      }
   } // queue

} // casual
