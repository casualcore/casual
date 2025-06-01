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

            namespace flush
            {
               template< typename M>
               void call( common::communication::ipc::inbound::Device& device, M&& request)
               {
                  log::debug( "request: ", request);

                  if( auto correlation = communication::ipc::flush::optional::send( device, instance::device(), request))
                  {
                     auto reply = common::message::reverse::type( request);
                     communication::device::blocking::receive( device, reply, correlation);
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
         void registration( common::communication::ipc::inbound::Device& device, Ability abilities)
         {
            Trace trace{ "domain::discovery::provider::registration"};

            message::discovery::api::provider::registration::Request message;
            message.process.ipc = device.connector().handle().ipc();
            message.process.pid = process::id();
            message.abilities = abilities;

            local::flush::call( device, message);
         }

         void registration( Ability abilities)
         {
            registration( communication::ipc::inbound::device(), abilities);
         }

      } // provider

      common::strong::correlation::id request( const Request& request)
      {
         Trace trace{ "domain::discovery::request"};
         log::debug( "request: ", request);
         
         return communication::ipc::flush::optional::send( instance::device(), request);
      }
      
      common::strong::correlation::id request( Send& multiplex, const Request& request)
      {
         Trace trace{ "domain::discovery::request"};
         log::debug( "request: ", request);
         
         return multiplex.send( instance::device(), request);
      }

      common::strong::correlation::id request(
         std::vector< std::string> services, 
         std::vector< std::string> queues, 
         common::strong::correlation::id correlation)
      {
         Trace trace{ "domain::discovery::request"};
         log::debug( "services: ", services, ", queues: ", queues);

         return communication::ipc::flush::optional::send( instance::device(), local::request( std::move( services), std::move( queues), correlation));
      }

      common::strong::correlation::id request( 
         Send& multiplex, 
         std::vector< std::string> services, 
         std::vector< std::string> queues, 
         common::strong::correlation::id correlation)
      {
         Trace trace{ "domain::discovery::request"};
         log::debug( "services: ", services, ", queues: ", queues);
         
         return multiplex.send( instance::device(), local::request( std::move( services), std::move( queues), correlation));
      }


      namespace topology
      {
         namespace direct
         {
            void update( Send& multiplex)
            {
               multiplex.send( instance::device(), message::discovery::topology::direct::Update{});
            }

            void update( Send& multiplex, const message::discovery::topology::direct::Update& message)
            {
               multiplex.send( instance::device(), message);
            }

         } // direct

         namespace implicit
         {
            void update( Send& multiplex, const message::discovery::topology::implicit::Update& message)
            {
               multiplex.send( instance::device(), message);
            }
         } // implicit
      } // topology

      namespace discoverable
      {
         void advertised( Send& multiplex)
         {
            multiplex.send( instance::device(), message::discovery::discoverable::Advertised{});
         }
      } // discoverable

      namespace rediscovery
      {
         common::strong::correlation::id request()
         {
            return communication::ipc::flush::optional::send( instance::device(), message::discovery::api::rediscovery::Request{ common::process::handle()});
         }
      } // rediscovery


   } // domain::discovery  
} // casual

