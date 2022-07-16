//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "domain/pending/message/send.h"
#include "domain/unittest/manager.h"


#include "common/message/service.h"

#include "common/communication/ipc.h"

namespace casual
{
   namespace domain::pending
   {
      TEST( domain_pending_message, spawn_terminate)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW( {
            auto domain = unittest::manager();
         });
      }

      TEST( domain_pending_message, eventually_message__expect_sent)
      {
         common::unittest::Trace trace;

         auto domain = unittest::manager();

         {
            common::message::service::lookup::Request message;
            message.requested = "foo";
            pending::message::send( common::process::handle(), message);
         }

         {
            common::message::service::lookup::Request message;
            common::communication::device::blocking::receive( common::communication::ipc::inbound::device(), message);

            EXPECT_TRUE( message.requested ==  "foo");
         }
      }

   } // domain::pending::send
} // casual
