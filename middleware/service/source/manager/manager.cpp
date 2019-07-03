//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
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

#include "common/communication/instance.h"

#include "serviceframework/log.h"


// std
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

                  return communication::ipc::call( communication::instance::outbound::domain::manager::device(), request).domain.service;
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

                  // Set the process variables so children can communicate with us.
                  common::environment::variable::process::set(
                        common::environment::variable::name::ipc::service::manager(),
                        common::process::handle());


                  // Get configuration from domain-manager
                  auto configuration = domain();


                  configure::services( state, configuration);


                  // Start forward
                  {
                     Trace trace{ "service::configure spawn forward"};

                     state.forward.pid = common::process::spawn( forward( settings), {});

                     state.forward = common::communication::instance::fetch::handle(
                           common::communication::instance::identity::forward::cache);

                     log::line( log, "forward: ", state.forward);

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

            // Terminate
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
                  // Prepare message-pump handlers

                  log::line( log, "prepare message-pump handlers");

                  auto handler = manager::handler( state);

                  // Connect to domain
                  communication::instance::connect( communication::instance::identity::service::manager);

                  log::line( log, "start message pump");

                  auto empty_callback = [&]()
                  {
                     // the input socket is empty, we can't know if there ever gonna be any more 
                     // messages to read, we need to send metric, if any...
                     if( state.metric && state.events.active< common::message::event::service::Calls>())
                        manager::handle::metric::send( state);
                  };

                  common::message::dispatch::empty::pump( 
                     handler, 
                     manager::ipc::device(), 
                     empty_callback);
               }

            } // message
         } // <unnamed>
      } // local

      void Manager::start()
      {
         local::message::pump( m_state);
      }

   } // service
} // casual





