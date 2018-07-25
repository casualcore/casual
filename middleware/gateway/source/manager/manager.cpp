//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "gateway/manager/manager.h"

#include "gateway/manager/handle.h"
#include "gateway/environment.h"
#include "gateway/transform.h"
#include "gateway/common.h"

#include "configuration/domain.h"
#include "configuration/message/transform.h"


#include "common/environment.h"
#include "common/exception/handle.h"
#include "common/communication/instance.h"

namespace casual
{
   using namespace common;

   namespace gateway
   {

      namespace manager
      {
         namespace local
         {
            namespace
            {
               manager::State configure( manager::Settings settings)
               {
                  Trace trace{ "gateway::manager::local::connect"};


                  //
                  // Set environment variable to make it easier for connections to get in
                  // touch with us
                  //
                  common::environment::variable::process::set(
                        common::environment::variable::name::ipc::gateway::manager(),
                        process::handle());


                  if( ! settings.configuration.empty())
                  {
                     return gateway::transform::state(
                           configuration::transform::configuration(
                                 configuration::domain::get( { settings.configuration})));
                  }


                  //
                  // Ask domain manager for configuration
                  //
                  common::message::domain::configuration::Request request;
                  request.process = process::handle();

                  return gateway::transform::state(
                        manager::ipc::device().call(
                              communication::instance::outbound::domain::manager::device(),
                              request).domain);

               }

               namespace dispatch
               {
                  auto inbound( State& state)
                  {
                     const auto descriptor = communication::ipc::inbound::handle().socket().descriptor();
                     state.directive.read.add( descriptor);

                     return communication::select::dispatch::create::reader(
                        descriptor,
                        [ handler = manager::handler( state)]( auto active) mutable 
                        {
                           handler( ipc::device().blocking_next());
                        }
                     );
                  }

                  auto listeners( State& state)
                  {
                     return handle::listen::Accept{ state};
                  }
               } // dispatch

            } // <unnamed>
         } // local

      } // manager






      Manager::Manager( manager::Settings settings)
        : m_state{ manager::local::configure( std::move( settings))}
      {
         Trace trace{ "gateway::Manager::Manager"};


      }

      Manager::~Manager()
      {
         Trace trace{ "gateway::Manager::~Manager"};

         try
         {
            // make sure we shutdown
            manager::handle::shutdown( m_state);
         }
         catch( ...)
         {
            common::exception::handle();
         }

      }

      void Manager::start()
      {
         Trace trace{ "gateway::Manager::start"};

         // boot outbounds
         {
            manager::handle::boot( m_state);
            m_state.runlevel = manager::State::Runlevel::online;
         }

         auto inbound = manager::local::dispatch::inbound( m_state);
         auto listeners = manager::local::dispatch::listeners( m_state);

         // Connect to domain
         communication::instance::connect( communication::instance::identity::gateway::manager);

         // start message pump
         communication::select::dispatch::pump( m_state.directive, inbound, listeners);
      }


   } // gateway


} // casual
