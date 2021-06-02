//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"
#include "common/unittest/file.h"

#include "configuration/user.h"

#include "common/serialize/create.h"

namespace casual
{
   using namespace common;

   namespace configuration::user::validate
   {
      namespace local
      {
         namespace
         {
            auto load_configuration = []( auto content)
            {
               auto path = common::unittest::file::temporary::content( ".yaml", std::move( content));
               
               user::Domain domain;

               common::file::Input stream{ path};
               auto archive = common::serialize::create::reader::consumed::from( stream);
               archive >> CASUAL_NAMED_VALUE( domain);
               archive.validate();
               domain.normalize();

               return domain;
            };

            namespace invalid::configuration
            {
               namespace gateway
               {
                  constexpr auto inbound_address = R"(
domain:
   gateway:
      inbound:
         groups:
            -  connections:
                  -  address: ""
)";

                  constexpr auto outbound_address = R"(
domain:
   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: ""
)";


                  constexpr auto reverse_inbound_address = R"(
domain:
   gateway:
      reverse:
         inbound:
            groups:
               -  connections:
                     - address: ""
)";

                  constexpr auto reverse_outbound_address = R"(
domain:
   gateway:
      reverse:
         outbound:
            groups:
               -  connections:
                     - address: ""
)";

               } // gateway

               namespace queue
               {
                  constexpr auto group_queue_name = R"(
domain:
   queue:
      groups:
         -  alias: A
            queues:
               -  name: ""
)";

               } // queue
            } // invalid::configuration      
               
         } // <unnamed>
      } // local

      struct invalid_configuration : public ::testing::TestWithParam< const char*> 
      {
         auto param() const { return ::testing::TestWithParam< const char*>::GetParam();}
      };

      INSTANTIATE_TEST_SUITE_P( configuration_user_validate,
         invalid_configuration,
         ::testing::Values(
            local::invalid::configuration::gateway::inbound_address,
            local::invalid::configuration::gateway::outbound_address,
            local::invalid::configuration::gateway::reverse_inbound_address,
            local::invalid::configuration::gateway::reverse_outbound_address,
            local::invalid::configuration::queue::group_queue_name
         ));

      TEST_P( invalid_configuration, load__expect_throw_invalid_configuration)
      {
         unittest::Trace trace;

         ASSERT_CODE({
            local::load_configuration( this->param());
         }, code::casual::invalid_configuration);

      }
      
   } // configuration::user::validate
} // casual