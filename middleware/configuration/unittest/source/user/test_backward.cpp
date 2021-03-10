//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"
#include "common/unittest/serialize.h"

#include "configuration/user.h"
#include "configuration/model/transform.h"

#include <optional>

namespace casual
{
   using namespace common;

   namespace configuration::user::backward
   {
      TEST( configuration_user_backward, gateway_connections)
      {
         unittest::Trace trace;

         constexpr auto user_yaml = R"(
gateway:
   connections:
      -  address: localhost:7001
         note: 7001
      -  address: localhost:7002
         note: 7002
      -  address: localhost:7003
         note: 7003
         services: [ a, b, c]
      -  address: localhost:7004
         note: 7004
         queues: [ a, b, c]
)";

         constexpr auto model_yaml = R"(
gateway:
   outbound:
      groups:
         -  alias: outbound
            connect: 1 # regular ( not reversed)
            order: 0
            note: 7001
            connections:
               -  address: localhost:7001
                  note: 7001
         -  alias: outbound.2
            connect: 1 # regular ( not reversed)
            order: 1 # generated order or each group
            note: 7002
            connections:
               -  address: localhost:7002
                  note: 7002
         -  alias: outbound.3
            connect: 1 # regular ( not reversed)
            order: 2
            note: 7003
            connections:
               -  address: localhost:7003
                  note: 7003
                  services: [ a, b, c]
         -  alias: outbound.4
            connect: 1 # regular ( not reversed)
            order: 3
            note: 7004
            connections:
               -  address: localhost:7004
                  note: 7004
                  queues: [ a, b, c]

)";

         auto user = model::transform( unittest::serialize::create::value< user::Domain>( "yaml", user_yaml));

         auto model = unittest::serialize::create::value< configuration::Model>( "yaml", model_yaml);

         EXPECT_TRUE( user == model) << " " << CASUAL_NAMED_VALUE( user) << '\n' << CASUAL_NAMED_VALUE( model);

      }

   } // configuration::user::backward
} // casual