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

                  // make sure we handle death of our children
                  signal::callback::registration< code::signal::child>( []()
                  {
                     algorithm::for_each( process::lifetime::ended(), []( auto& exit)
                     {
                        manager::handle::process::exit( exit);
                     });
                  }); 

                  // Set environment variable to make it easier for connections to get in
                  // touch with us
                  common::environment::variable::process::set(
                        common::environment::variable::name::ipc::gateway::manager,
                        process::handle());


                  if( ! settings.configuration.empty())
                  {
                     return gateway::transform::state(
                           configuration::transform::configuration(
                                 configuration::domain::get( { settings.configuration})));
                  }

                  // Ask domain manager for configuration
                  common::message::domain::configuration::Request request;
                  request.process = process::handle();

                  return gateway::transform::state(
                     communication::ipc::call(
                        communication::instance::outbound::domain::manager::device(),
                        request).domain);

               }

               namespace dispatch
               {
                  using handler_type = typename communication::ipc::inbound::Device::handler_type;
                  struct Inbound
                  {
                     Inbound( State& state) : m_handler( manager::handler( state)) 
                     {
                        state.directive.read.add( communication::ipc::inbound::handle().socket().descriptor());
                     }
                     
                     auto descriptor() const { return communication::ipc::inbound::handle().socket().descriptor();}

                     void operator () ( strong::file::descriptor::id descriptor)
                     {
                        consume();
                     }

                     bool consume()
                     {
                        return m_handler( communication::ipc::non::blocking::next( ipc::device()));
                     } 
                     handler_type m_handler;
                  };

                  auto inbound( State& state)
                  {
                     return Inbound( state);
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
         manager::handle::boot( m_state);

         auto inbound = manager::local::dispatch::inbound( m_state);
         auto listeners = manager::local::dispatch::listeners( m_state);

         // Connect to domain
         communication::instance::connect( communication::instance::identity::gateway::manager);

         m_state.runlevel = manager::State::Runlevel::online;

         log::line( log::category::information, "casual-gateway-manager is online");

         // start message pump
         communication::select::dispatch::pump( m_state.directive, inbound, listeners);
      }

   } // gateway
} // casual
