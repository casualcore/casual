//!
//! casual 
//!

#include <gtest/gtest.h>

#include "domain/delay/message.h"

#include "common/mockup/process.h"
#include "common/mockup/domain.h"

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
                  common::mockup::domain::Broker broker;
               };

               namespace process
               {
                  struct Delay : common::mockup::Process
                  {
                     Delay() : common::mockup::Process{ "./bin/casual-delay-message", {}} {}

                  };

               } // process

            } // <unnamed>
         } // local


         TEST( casual_domain_delay, spawn_terminate)
         {
            EXPECT_NO_THROW( {
               local::Domain domain;
               local::process::Delay delay;
            });
         }


         TEST( casual_domain_delay, send_delayed_message__10ms__expect_to_receive_after_at_least_10ms)
         {
            local::Domain domain;
            local::process::Delay delay;

            common::communication::ipc::Helper ipc;

            auto id = common::uuid::make();

            auto start = common::platform::clock_type::now();

            {
               common::message::lookup::process::Request message;
               message.identification = id;
               message::send( message, ipc.id(), std::chrono::milliseconds{ 10});
            }

            {
               common::message::lookup::process::Request message;
               ipc.blocking_receive( message);

               EXPECT_TRUE( message.identification == id);
               EXPECT_TRUE( common::platform::clock_type::now() - start > std::chrono::milliseconds{ 10});

            }

         }

         TEST( casual_domain_delay, send_delayed_message__0ms__expect_to_receive_within_10ms)
         {
            local::Domain domain;
            local::process::Delay delay;

            common::communication::ipc::Helper ipc;

            auto id = common::uuid::make();

            auto start = common::platform::clock_type::now();

            {
               common::message::lookup::process::Request message;
               message.identification = id;
               message::send( message, ipc.id(), std::chrono::milliseconds{ 0});
            }

            {
               common::message::lookup::process::Request message;
               ipc.blocking_receive( message);

               EXPECT_TRUE( message.identification == id);
               EXPECT_TRUE( common::platform::clock_type::now() - start < std::chrono::milliseconds{ 10});

            }

         }




      } // delay
   } // domain
} // casual
