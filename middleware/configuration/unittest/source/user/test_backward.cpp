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
domain:
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

         // internal model
         constexpr auto model_yaml = R"(
gateway:
   outbound:
      groups:
         -  alias: outbound
            order: 1
            connect: 1 # regular ( not reversed)
            # user normalize add this note...
            note: "transformed from DEPRECATED domain.gateway.connections[]"
            connections:
               -  address: localhost:7001
                  note: 7001
         -  alias: outbound.2
            order: 2
            connect: 1 # regular ( not reversed)
            note: "transformed from DEPRECATED domain.gateway.connections[]"
            connections:
               -  address: localhost:7002
                  note: 7002
         -  alias: outbound.3
            order: 3
            connect: 1 # regular ( not reversed)
            note: "transformed from DEPRECATED domain.gateway.connections[]"
            connections:
               -  address: localhost:7003
                  note: 7003
                  services: [ a, b, c]
         -  alias: outbound.4
            order: 4
            connect: 1 # regular ( not reversed)
            note: "transformed from DEPRECATED domain.gateway.connections[]"
            connections:
               -  address: localhost:7004
                  note: 7004
                  queues: [ a, b, c]

)";

         // we need to 'normalize' both user and internal. user -> _backward to current (user) : internal -> alias mapping stuff. 
         auto user = normalize( model::transform( normalize( unittest::serialize::create::value< user::Model>( "yaml", user_yaml))));

         auto model = unittest::serialize::create::value< configuration::Model>( "yaml", model_yaml);

         EXPECT_TRUE( user.gateway == model.gateway) << " " << CASUAL_NAMED_VALUE( user.gateway) << '\n' << CASUAL_NAMED_VALUE( model.gateway);

      }

   } // configuration::user::backward
} // casual