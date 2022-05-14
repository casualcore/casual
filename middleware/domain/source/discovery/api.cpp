//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/discovery/api.h"
#include "domain/discovery/common.h"
#include "domain/discovery/instance.h"
#include "domain/message/discovery.h"

#include "common/communication/instance.h"
#include "common/communication/ipc/flush/send.h"

namespace casual
{
   using namespace common;
   namespace domain::discovery
   {
      namespace local
      {
         namespace
         {
            namespace instance
            {
               auto& device()
               {
                  static communication::instance::outbound::detail::optional::Device device{ discovery::instance::identity};
                  return device;
               }
            } // instance

            namespace flush
            {
               template< typename M>
               void call( M&& request)
               {
                  log::line( verbose::log, "request: ", request);

                  if( auto correlation = communication::ipc::flush::optional::send( local::instance::device(), request))
                  {
                     auto reply = common::message::reverse::type( request);
                     communication::device::blocking::receive( communication::ipc::inbound::device(), reply, correlation);
                  }
               }
            } // flush
         } // <unnamed>
      } // local

      namespace provider
      {
         void registration( const common::process::Handle& process, common::Flags< Ability> abilities)
         {
            Trace trace{ "domain::discovery::provider::registration"};

            message::discovery::api::provider::registration::Request message{ process};
            message.abilities = abilities;

            local::flush::call( message);
         }

         void registration( common::Flags< Ability> abilities)
         {
            registration( process::handle(), abilities);
         }

      } // provider
      
      common::strong::correlation::id request( const Request& request)
      {
         Trace trace{ "domain::discovery::request"};
         log::line( verbose::log, "request: ", request);
         
         return communication::ipc::flush::optional::send( local::instance::device(), request);
      }

      common::strong::correlation::id request( std::vector< std::string> services, std::vector< std::string> queues, common::strong::correlation::id correlation)
      {
         Trace trace{ "domain::discovery::request"};
         log::line( verbose::log, "services: ", services, ", queues: ", queues);

         message::discovery::api::Request request{ common::process::handle()};
         request.correlation = correlation;
         request.content = message::discovery::request::Content{ std::move( services), std::move( queues)};

         return communication::ipc::flush::optional::send( local::instance::device(), request);
      }

      namespace topology
      {
         void update()
         {
            update( message::discovery::topology::Update{});
         }

         void update( const message::discovery::topology::Update& message)
         {
            Trace trace{ "domain::discovery::rediscovery::topology::update"};
            communication::ipc::flush::optional::send( local::instance::device(), message);
         }
      } // topology

      namespace rediscovery
      {
         common::strong::correlation::id request()
         {
            return communication::ipc::flush::optional::send( local::instance::device(), message::discovery::api::rediscovery::Request{ common::process::handle()});
         }
      } // rediscovery


   } // domain::discovery  
} // casual

