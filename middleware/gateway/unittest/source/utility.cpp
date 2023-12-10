//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/unittest/utility.h"
#include "gateway/manager/admin/server.h"

#include "serviceframework/service/protocol/call.h"

#include "common/communication/ipc.h"
#include "common/unittest.h"

namespace casual
{
   using namespace common;
   namespace gateway::unittest
   {
      manager::admin::model::State state()
      {
         common::unittest::service::wait::until::advertised( manager::admin::service::name::state);
         serviceframework::service::protocol::binary::Call call;
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
      
   } // gateway::unittest
   
} // casual