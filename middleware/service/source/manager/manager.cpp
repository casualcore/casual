//!
//! casual
//!

#include "service/manager/manager.h"

#include "service/manager/handle.h"
#include "service/manager/admin/server.h"

#include "service/transform.h"
#include "service/common.h"


#include "common/environment.h"
#include "common/domain.h"

#include "common/message/dispatch.h"
#include "common/message/domain.h"
#include "common/message/handle.h"
#include "common/process.h"
#include "common/domain.h"


#include "sf/log.h"


#include <xatmi.h>

#include <fstream>
#include <algorithm>
#include <iostream>





namespace casual
{
   using namespace common;

	namespace service
	{

      namespace local
      {
         namespace
         {

            namespace configure
            {
               std::string forward( const manager::Settings& settings)
               {
                  if( settings.forward.empty())
                  {
                     return environment::directory::casual() + "/bin/casual-service-forward";
                  }
                  return settings.forward;
               }

               common::message::domain::configuration::service::Manager domain()
               {
                  common::message::domain::configuration::Request request;
                  request.process = process::handle();

                  return communication::ipc::call( communication::ipc::domain::manager::device(), request).domain.service;
               }

               void services( manager::State& state, const common::message::domain::configuration::service::Manager& configuration)
               {
                  Trace trace{ "service::local::configure::services"};

                  state.default_timeout = configuration.default_timeout;

                  for( auto& config : configuration.services)
                  {
                     common::message::service::call::Service service;

                     service.timeout = config.timeout;
                     service.name = config.name;

                     state.services.emplace( std::make_pair( config.name, std::move( service)));

                  }

               }

               manager::State state( const manager::Settings& settings)
               {
                  manager::State state;

                  //
                  // Set the process variables so children can communicate with us.
                  //
                  common::environment::variable::process::set(
                        common::environment::variable::name::ipc::service::manager(),
                        common::process::handle());


                  //
                  // Get configuration from domain-manager
                  //
                  auto configuration = domain();


                  configure::services( state, configuration);


                  //
                  // Start forward
                  //
                  {
                     Trace trace{ "service::configure spawn forward"};

                     state.forward.pid = common::process::spawn( forward( settings), {});

                     state.forward = common::process::instance::fetch::handle(
                           common::process::instance::identity::forward::cache());

                     log << "forward: " << state.forward << '\n';

                  }

                  return state;
               }

            } // configure

         } // <unnamed>
      } // local



      Manager::Manager( manager::Settings&& settings) : m_state{ local::configure::state( std::move( settings))}
      {
         Trace trace{ "service::Manager::Manager ctor"};

      }



      Manager::~Manager()
      {
         try
         {
            Trace trace{ "service::Manager::Manager dtor"};

            //
            // Terminate
            //
            process::terminate( m_state.forward);
         }
         catch( ...)
         {
            common::exception::handle();
         }
      }

      namespace local
      {
         namespace
         {
            namespace message
            {
               void pump( manager::State& state)
               {
                  //
                  // Prepare message-pump handlers
                  //

                  log << "prepare message-pump handlers\n";


                  auto handler = manager::handler( state);


                  //
                  // Connect to domain
                  //
                  process::instance::connect( process::instance::identity::service::manager());


                  log << "start message pump\n";


                  while( true)
                  {
                     if( state.pending.replies.empty())
                     {
                        handler( manager::ipc::device().blocking_next());
                     }
                     else
                     {
                        signal::handle();
                        signal::thread::scope::Block block;

                        //
                        // Send pending replies
                        //
                        {

                           verbose::log << "pending replies: " << range::make( state.pending.replies) << '\n';

                           auto replies = std::exchange( state.pending.replies, {});

                           auto remain = std::get< 1>( common::algorithm::partition(
                                 replies,
                                 common::message::pending::sender(
                                       communication::ipc::policy::non::Blocking{},
                                       manager::ipc::device().error_handler())));

                           algorithm::move( remain, state.pending.replies);
                        }

                        //
                        // Take care of service dispatch
                        //
                        {
                           //
                           // If we've got pending that is 'never' sent, we still want to
                           // do a lot of service stuff. Hence, if we got into an 'error state'
                           // we'll still function...
                           //
                           // TODO: Should we have some sort of TTL for the pending?
                           //
                           auto count = common::platform::batch::service::pending;

                           while( handler( manager::ipc::device().non_blocking_next()) && count-- > 0)
                              ;
                        }

                     }
                  }
               }

            } // message
         } // <unnamed>
      } // local

      void Manager::start()
      {
         try
         {
            local::message::pump( m_state);
         }
         catch( ...)
         {
            common::exception::handle();
         }
      }

	} // service
} // casual





