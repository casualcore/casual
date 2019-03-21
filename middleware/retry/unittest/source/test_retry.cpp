//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <gtest/gtest.h>
#include "common/unittest.h"

#include "retry/send/message.h"

#include "common/mockup/process.h"
#include "common/mockup/domain.h"

#include "common/message/service.h"

#include "common/communication/ipc.h"

namespace casual
{

   namespace retry
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

               namespace process
               {
                  struct Retry : common::mockup::Process
                  {
                     Retry() : common::mockup::Process{ "./bin/casual-retry-send", {}} {}

                  };

               } // process

            } // <unnamed>
         } // local


         TEST( retry_send, spawn_terminate)
         {
            common::unittest::Trace trace;

            EXPECT_NO_THROW( {
               local::Domain domain;
               local::process::Retry retry;
            });
         }



         TEST( retry_send, send_retry_message__expect_sent)
         {
            common::unittest::Trace trace;

            local::Domain domain;
            local::process::Retry retry;

            common::communication::ipc::Helper ipc;

            {
               common::message::service::lookup::Request message;
               message.requested = "foo";
               message::send( message, common::process::handle());
            }

            {
               common::message::service::lookup::Request message;
               ipc.blocking_receive( message);

               EXPECT_TRUE( message.requested ==  "foo");
            }
         }

      } // send
   } // retry
} // casual
