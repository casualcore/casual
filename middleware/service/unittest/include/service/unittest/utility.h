//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/unittest.h"

#include "service/manager/admin/model.h"

#include "common/message/service.h"
#include "common/process.h"

#include <string>
#include <vector>

namespace casual
{
   namespace service::unittest
   {
      //! advertise `services` to service-manager as current process
      void advertise( std::vector< std::string> services);

      //! advertise `services` to service-manager as provided process
      void advertise( std::vector< std::string> services, const common::process::Handle& handle);

      //! unadvertise `services` to service-manager as current process
      void unadvertise( std::vector< std::string> services);

      namespace concurrent
      {
         //! advertise concurrent/remote `services` to service-manager as current process
         void advertise( std::vector< std::string> services);

         //! unadvertise concurrent/remote `services` to service-manager as current process
         void unadvertise( std::vector< std::string> services);
      } // concurrent

      common::message::service::lookup::Reply lookup( std::string service);

      namespace send
      {
         namespace wait
         {
            //! sends lookup with _wait_ that will block until the service is available, then 
            //! sends a request to the service.
            [[nodiscard]] auto request( std::string service, platform::binary::type payload) -> common::strong::correlation::id;
            
         } // wait
         

         //! sends ack to service-manager
         //! @{ 
         void ack( const common::message::service::call::callee::Request& request);
         void ack( const common::message::service::lookup::Reply& lookup);
         //! @}

      } // send

      namespace server
      {
         // emulate a server reply. receives a request, send echo reply.
         // and send ACK to SM.
         common::strong::correlation::id echo( const common::strong::correlation::id& correlation);
         
      } // server

      manager::admin::model::State state();

      namespace fetch
      {
         constexpr auto until = common::unittest::fetch::until( &unittest::state);

         namespace predicate
         {
            inline auto instances( std::string_view service, platform::size::type count)
            {
               return [ service, count]( auto& state)
               {
                  if( auto found = common::algorithm::find( state.services, service))
                     return found->instances.size() == count;

                  return false;
               };
            }
         } // predicate
      }



   
   } // common::unittest
} // casual