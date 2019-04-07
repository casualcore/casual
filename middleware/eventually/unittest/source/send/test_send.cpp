//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <gtest/gtest.h>
#include "common/unittest.h"

#include "eventually/send/message.h"
#include "eventually/send/unittest/process.h"

#include "common/mockup/process.h"
#include "common/mockup/domain.h"

#include "common/message/service.h"

#include "common/communication/ipc.h"

namespace casual
{

   namespace eventually
   {
      namespace send
      {
         namespace local
         {
            namespace
            {
               struct Domain
               {
                  common::mockup::domain::Manager manager;
               };
            } // <unnamed>
         } // local

         TEST( eventually_send, spawn_terminate)
         {
            common::unittest::Trace trace;

            EXPECT_NO_THROW( {
               local::Domain domain;
               unittest::Process send;
            });
         }



         TEST( eventually_send, eventually_message__expect_sent)
         {
            common::unittest::Trace trace;

            local::Domain domain;
            unittest::Process send;

            common::communication::ipc::Helper ipc;

            {
               common::message::service::lookup::Request message;
               message.requested = "foo";
               eventually::send::message( common::process::handle(), message);
            }

            {
               common::message::service::lookup::Request message;
               ipc.blocking_receive( message);

               EXPECT_TRUE( message.requested ==  "foo");
            }
         }

      } // send
   } // eventually
} // casual
