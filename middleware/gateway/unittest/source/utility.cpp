//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/unittest/utility.h"
#include "gateway/manager/admin/server.h"
#include "gateway/message.h"

#include "service/protocol/call.h"
#include "service/unittest/utility.h"

#include "common/communication/ipc.h"


namespace casual
{
   using namespace common;
   namespace gateway::unittest
   {
      manager::admin::model::State state()
      {
         casual::service::unittest::wait::until::advertised( manager::admin::service::name::state); 
         casual::service::protocol::binary::Call call;
         auto reply = call( manager::admin::service::name::state);
         return reply.extract< manager::admin::model::State>();
      }

      namespace inbound
      {
         std::optional< manager::admin::model::inbound::Group> group( const manager::admin::model::State& state, std::string_view alias)
         {
            if( auto found = algorithm::find( state.inbound.groups, alias))
               return *found;

            return {};
         }
      } // inbound

      namespace outbound
      {
         std::optional< manager::admin::model::outbound::Group> group( const manager::admin::model::State& state, std::string_view alias)
         {
            if( auto found = algorithm::find( state.outbound.groups, alias))
               return *found;

            return {};
         }
      } // outbound


      common::process::Handle group( const manager::admin::model::State& state, std::string_view alias)
      {
         if( auto found = inbound::group( state, alias))
            return found->process;

         if( auto found = outbound::group( state, alias))
            return found->process;

         return {};         
      }

      namespace tcp::connect
      {
         namespace local
         {
            namespace
            {
               auto eventually_connect( std::string_view address)
               {
                  communication::Socket socket;
                  common::unittest::eventually::succeed( [ &socket, address]()
                  {
                     socket = communication::tcp::connect( std::string{ address});
                     return predicate::boolean( socket);
                  });
                  return socket;
               };
   
            } // <unnamed>
         } // local

         common::communication::tcp::Duplex out( std::string_view address, message::protocol::Version version, common::domain::Identity domain)
         {
            common::communication::tcp::Duplex device{ local::eventually_connect( address)};

            {
               gateway::message::domain::connect::Request request;
               request.domain = domain;
               request.versions.push_back( version);

               auto reply = communication::device::call( device, request, device);

               if( reply.version != version)
                  code::raise::error( code::casual::invalid_semantics, "expected version: ", version, " but got: ", reply.version);
            }

            return device;
         }

         common::communication::tcp::Duplex in( std::string_view address, message::protocol::Version version, common::domain::Identity domain)
         {
            common::communication::tcp::Duplex device{ local::eventually_connect( address)};

            auto request = communication::device::receive< gateway::message::domain::connect::Request>( device);

            if( ! algorithm::find( request.versions, version))
               code::raise::error( code::casual::invalid_semantics, "could not find version: ", version, " in ", request.versions);
            
            auto reply = common::message::reverse::type( request);
            reply.domain = domain;
            reply.version = version;

            communication::device::blocking::send( device, reply);

            return device;
         }
         
      } // tcp::connect
      
   } // gateway::unittest
   
} // casual