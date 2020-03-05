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
               namespace forward
               {
                  std::string path( const manager::Settings& settings)
                  {
                     if( settings.forward.empty())
                        return environment::directory::casual() + "/bin/casual-service-forward";

                     return settings.forward;
                  }
               } // forward

               auto domain()
               {
                  Trace trace{ "service::local::configure::domain"};

                  return communication::ipc::call( 
                     communication::instance::outbound::domain::manager::device(), 
                     common::message::domain::configuration::Request{ process::handle()}).domain.service;
               }

               auto state( manager::Settings settings)
               {
                  Trace trace{ "service::local::configure::state"};

                  // Set the process variables so children can find us easier.
                  common::environment::variable::process::set(
                     common::environment::variable::name::ipc::service::manager,
                     common::process::handle());

                  // Get configuration from domain-manager
                  manager::State state{ domain()};

                  // Start forward
                  {
                     Trace trace{ "service::configure spawn forward"};

                     state.forward.pid = common::process::spawn( forward::path( settings), {});
                     state.forward = common::communication::instance::fetch::handle(
                           common::communication::instance::identity::forward::cache);

                     log::line( log, "forward: ", state.forward);
                  }

                  log::line( verbose::log, "state: ", state);

                  return state;
               }
            } // configure
         } // <unnamed>
      } // local

      Manager::Manager( manager::Settings&& settings) 
         : m_state{ local::configure::state( std::move( settings))}
      {
         Trace trace{ "service::Manager::Manager ctor"};
      }

      Manager::~Manager()
      {
         exception::guard( [&]()
         {
            Trace trace{ "service::Manager::Manager dtor"};
            // Terminate
            process::terminate( m_state.forward);
         });
      }

      void Manager::start()
      {
         Trace trace{ "service::Manager::start"};

         auto handler = manager::handler( m_state);

         // Connect to domain
         communication::instance::connect( communication::instance::identity::service::manager);

         log::line( log, "start message pump");

         auto empty_callback = [&]()
         {
            // the input socket is empty, we can't know if there ever gonna be any more 
            // messages to read, we need to send metric, if any...
            if( m_state.metric && m_state.events.active< common::message::event::service::Calls>())
               manager::handle::metric::send( m_state);
         };

         common::message::dispatch::empty::pump( 
            handler, 
            manager::ipc::device(), 
            empty_callback);
      }

   } // service
} // casual





