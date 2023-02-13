//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "http/outbound/state.h"
#include "http/outbound/configuration.h"
#include "http/outbound/transform.h"
#include "http/outbound/handle.h"
#include "http/outbound/request.h"

#include "domain/discovery/api.h"

#include "common/instance.h"
#include "common/communication/device.h"
#include "common/communication/instance.h"
#include "common/argument.h"
#include "common/exception/guard.h"
#include "common/serialize/macro.h"

namespace casual
{
   using namespace common;

   namespace http::outbound
   {
      namespace local
      {
         namespace
         {
            struct Settings
            {
               std::vector< std::string> configurations;

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( configurations);
               )
            };

            auto initialize( Settings settings)
            {
               Trace trace{ "http::outbound::local::initialize"};

               auto state = transform::configuration( configuration::load( settings.configurations));

               // advertise
               {
                  common::message::service::concurrent::Advertise message{ process::handle()};
                  message.alias = instance::alias();

                  // lowest possible order
                  message.order = 0;

                  algorithm::transform( state.lookup, message.services.add, []( auto& l){
                     message::service::concurrent::advertise::Service service;
                     service.category = "http";
                     service.name = l.first;
                     service.transaction = service::transaction::Type::none;
                     return service;
                  });

                  log::line( verbose::log, "advertise: ", message);

                  communication::device::blocking::send( communication::instance::outbound::service::manager::device(), message);
               }

               // register to answer external discoveries.
               casual::domain::discovery::provider::registration( casual::domain::discovery::provider::Ability::discover);

               // connect to domain
               common::communication::instance::whitelist::connect();

               return state;
            }

            void run( State state)
            {
               Trace trace{ "http::outbound::local::run"};

               auto internal = [dispatch = handle::internal::create( state)]( auto policy) mutable
               {
                  dispatch( communication::ipc::inbound::device().next( policy));
               };

               // callback that is called if curl has stuff ready
               auto external = handle::external::reply( state);

               while( true)
               {
                  if( state.pending.requests)
                  {
                     log::line( verbose::log, "state.pending.requests.size(): ", state.pending.requests.size());
                     request::blocking::dispath( state, internal, external);
                  }
                  else
                  {
                     // we've got no pending request, we only have to listen to inbound
                     internal( communication::device::policy::blocking( communication::ipc::inbound::device()));
                  } 
               }
            }

            void main( int argc, char **argv)
            {
               Trace trace{ "http::outbound::local::main"};

               Settings settings;

               argument::Parse{ "http outbound",
                  argument::Option( std::tie( settings.configurations), argument::option::keys( { "--configuration"}, { "--configuration-files"}), "configuration glob patterns")
               }( argc, argv);

               log::line( verbose::log, "settings: ", settings);

               run( initialize( std::move( settings)));
            }
            
         } // <unnamed>
      } // local


   } // http::outbound

} // casual

int main( int argc, char **argv)
{
   return casual::common::exception::main::log::guard( [=]()
   {
      casual::http::outbound::local::main( argc, argv);
   });

}



