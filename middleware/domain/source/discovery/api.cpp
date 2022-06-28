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

            auto request( std::vector< std::string> services, std::vector< std::string> queues, common::strong::correlation::id correlation)
            {
               message::discovery::api::Request request{ common::process::handle()};
               request.correlation = correlation;
               request.content = message::discovery::request::Content{ std::move( services), std::move( queues)};
               return request;
            }
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
      
      common::strong::correlation::id request( Send& multiplex, const Request& request)
      {
         Trace trace{ "domain::discovery::request"};
         log::line( verbose::log, "request: ", request);
         
         return multiplex.send( local::instance::device(), request);
      }

      common::strong::correlation::id request(
         std::vector< std::string> services, 
         std::vector< std::string> queues, 
         common::strong::correlation::id correlation)
      {
         Trace trace{ "domain::discovery::request"};
         log::line( verbose::log, "services: ", services, ", queues: ", queues);

         return communication::ipc::flush::optional::send( local::instance::device(), local::request( std::move( services), std::move( queues), correlation));
      }

      common::strong::correlation::id request( 
         Send& multiplex, 
         std::vector< std::string> services, 
         std::vector< std::string> queues, 
         common::strong::correlation::id correlation)
      {
         Trace trace{ "domain::discovery::request"};
         log::line( verbose::log, "services: ", services, ", queues: ", queues);
         
         return multiplex.send( local::instance::device(), local::request( std::move( services), std::move( queues), correlation));
      }


      namespace topology
      {
         namespace direct
         {
            void update( Send& multiplex)
            {
               multiplex.send( local::instance::device(), message::discovery::topology::direct::Update{});
            }

            void update( Send& multiplex, const message::discovery::topology::direct::Update& message)
            {
               multiplex.send( local::instance::device(), message);
            }

         } // direct

         namespace implicit
         {
            void update( Send& multiplex, const message::discovery::topology::implicit::Update& message)
            {
               multiplex.send( local::instance::device(), message);
            }
         } // implicit
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

