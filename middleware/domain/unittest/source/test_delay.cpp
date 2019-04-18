//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <gtest/gtest.h>
#include "common/unittest.h"

#include "domain/manager/unittest/process.h"

#include "domain/delay/message.h"

#include "common/message/domain.h"
#include "common/communication/ipc.h"

namespace casual
{

   namespace domain
   {
      namespace delay
      {
         namespace local
         {
            namespace
            {
               struct Domain
               {
                  domain::manager::unittest::Process manager{ { Domain::configuration}};
                  
                  static constexpr auto configuration = R"(
domain:
   name: delay-domain

   servers: 
      - path: "./bin/casual-delay-message"
)";
               };

            } // <unnamed>
         } // local


         TEST( casual_domain_delay, spawn_terminate)
         {
            common::unittest::Trace trace;

            EXPECT_NO_THROW( {
               local::Domain domain;
            });
         }


         TEST( casual_domain_delay, send_delayed_message__10ms__expect_to_receive_after_at_least_10ms)
         {
            common::unittest::Trace trace;

            local::Domain domain;

            common::communication::ipc::Helper ipc;

            auto id = common::uuid::make();

            auto start = common::platform::time::clock::type::now();

            {
               common::message::domain::process::lookup::Request message;
               message.identification = id;
               message::send( message, ipc.ipc(), std::chrono::milliseconds{ 10});
            }

            {
               common::message::domain::process::lookup::Request message;
               ipc.blocking_receive( message);

               EXPECT_TRUE( message.identification == id);
               EXPECT_TRUE( common::platform::time::clock::type::now() - start > std::chrono::milliseconds{ 10});

            }

         }

         TEST( casual_domain_delay, send_delayed_message__0ms__expect_to_receive_within_100ms)
         {
            common::unittest::Trace trace;

            local::Domain domain;

            common::communication::ipc::Helper ipc;

            auto id = common::uuid::make();


            {
               common::message::domain::process::lookup::Request message;
               message.identification = id;
               message::send( message, ipc.ipc(), std::chrono::milliseconds{ 0});
            }

            auto start = common::platform::time::clock::type::now();

            {
               common::message::domain::process::lookup::Request message;
               ipc.blocking_receive( message);

               EXPECT_TRUE( message.identification == id);
               EXPECT_TRUE( common::platform::time::clock::type::now() - start < std::chrono::milliseconds{ 100});

            }

         }

      } // delay
   } // domain
} // casual
