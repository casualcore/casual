//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "casual/manager/service/policy.h"
#include "casual/manager/service/handle.h"

#include "common/message/domain.h"
#include "common/log.h"
#include "common/communication/instance.h"
#include "common/instance.h"

#include <ranges>

namespace casual
{
   namespace manager::service::policy
   {
      using namespace common;
      

      void Default::send_ack( const common::message::service::call::ACK& ack)
      {
         Trace trace{ "manager::service::policy::Default::send_ack"};

         communication::device::blocking::send( communication::instance::outbound::service::manager::device(), ack);
      }

      void Default::initialize( const context::State& state)
      {
         Trace trace{ "manager::service::policy::Default::initialize"};

         // send a lookup to DM
         common::message::domain::process::lookup::Request message{ process::handle()};
         message.identification = communication::instance::identity::service::manager.id;
         message.directive = decltype( message.directive)::wait;

         communication::device::blocking::send( communication::instance::outbound::domain::manager::device(), message);
      }


      void Default::advertise( const context::State& state, const common::message::domain::process::lookup::Reply& message)
      {
         Trace trace{ "manager::service::policy::Default::advertise"};

         if( message.identification != communication::instance::identity::service::manager.id)
            return;

         if( ! message.process)
            return;

         auto advertise = service::advertise::transform( state.services | std::views::values);

         if( advertise.sequential)
            communication::device::blocking::send( message.process.ipc, *advertise.sequential);
         if( advertise.concurrent)
            communication::device::blocking::send( message.process.ipc, *advertise.concurrent);
      }

   } // manager::service::policy
   
} // casual
