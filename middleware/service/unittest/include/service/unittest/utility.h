//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/unittest.h"

#include "common/message/service.h"
#include "service/manager/admin/model.h"

#include <string>
#include <vector>

namespace casual
{
   namespace service::unittest
   {
      //! advertise `services` to service-manager as `process`
      void advertise( std::vector< std::string> services, const common::process::Handle& process = common::process::handle());

      //! unadvertise `services` to service-manager as `process`
      void unadvertise( std::vector< std::string> services, const common::process::Handle& process = common::process::handle());

      namespace concurrent
      {
         //! advertise concurrent/remote `services` to service-manager as `process`
         void advertise( std::vector< std::string> services, const common::process::Handle& process = common::process::handle());

         //! unadvertise concurrent/remote `services` to service-manager as `process`
         void unadvertise( std::vector< std::string> services, const common::process::Handle& process = common::process::handle());
      } // concurrent

      namespace send
      {
         //! sends ack to service-manager
         //! @{ 
         void ack( const common::message::service::call::callee::Request& request);
         void ack( const common::message::service::lookup::Reply& lookup);
         void ack( const common::message::service::lookup::Reply& lookup, const common::transaction::ID& trid);
         //! @}

         namespace concurrent
         {
            //! sends ack to service-manager for concurrent service
            void ack( const common::message::service::lookup::Reply& lookup, const common::transaction::ID& trid);
         } // concurrent

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