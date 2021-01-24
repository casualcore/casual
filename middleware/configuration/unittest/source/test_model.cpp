//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"
#include "common/unittest/file.h"

#include "configuration/model/transform.h"
#include "configuration/model/load.h"
#include "configuration/user.h"
#include "configuration/example/domain.h"



namespace casual
{
   using namespace common;
   namespace configuration
   {
      namespace local
      {
         namespace
         {
            auto configuration = []( auto content)
            {
               auto path = unittest::file::temporary::content( ".yaml", std::move( content));
               return configuration::model::load( { path.path()});
            };
         } // <unnamed>
      } // local


      TEST( configuration_model_transform, default_domain)
      {
         common::unittest::Trace trace;

         auto model = model::transform( configuration::user::Domain{});

         EXPECT_TRUE( model.domain.name.empty());
      }

      TEST( configuration_model_transform, simple_configuration)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain:
  name: model
  
)";
         
         auto model = local::configuration( configuration);

         EXPECT_TRUE( model.domain.name == "model");
      }


      TEST( configuration_model_transform, example_configuration_roundtrip__expect_equality)
      {
         common::unittest::Trace trace;

         const auto origin = model::transform( example::domain());

         // roundtrip
         const auto result = model::transform( model::transform( origin));

         EXPECT_TRUE( origin.domain.environment == result.domain.environment) << CASUAL_NAMED_VALUE( origin.domain.environment) << "\n" << CASUAL_NAMED_VALUE( result.domain.environment);
         EXPECT_TRUE( origin.domain.servers == result.domain.servers) << CASUAL_NAMED_VALUE( origin.domain.servers) << "\n" << CASUAL_NAMED_VALUE( result.domain.servers);
         EXPECT_TRUE( origin.domain.executables == result.domain.executables);
         EXPECT_TRUE( origin.domain.groups == result.domain.groups);
         EXPECT_TRUE( origin.service == result.service) << CASUAL_NAMED_VALUE( origin.service) << "\n" << CASUAL_NAMED_VALUE( result.service);
         EXPECT_TRUE( origin.transaction == result.transaction) << CASUAL_NAMED_VALUE( origin.transaction) << "\n" << CASUAL_NAMED_VALUE( result.transaction);
         EXPECT_TRUE( origin.gateway == result.gateway) << CASUAL_NAMED_VALUE( origin.gateway) << "\n" << CASUAL_NAMED_VALUE( result.gateway);
         EXPECT_TRUE( origin.queue == result.queue) << CASUAL_NAMED_VALUE( origin.queue) << "\n" << CASUAL_NAMED_VALUE( result.queue);
         

         EXPECT_TRUE( origin == result) << "\n" << CASUAL_NAMED_VALUE( origin) << "\n " << CASUAL_NAMED_VALUE( result);
      }

      TEST( configuration_model_transform, service_restrition)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain:
  name: model
  servers:
    - path: a
      restrictions: [ a1, a2, a3]
    - path: b
    - path: c
      restrictions: [ c1, c2]
)";
         
         auto model = local::configuration( configuration);

         EXPECT_TRUE( model.domain.name == "model");
         ASSERT_TRUE( model.domain.servers.size() == 3) << CASUAL_NAMED_VALUE( model);

         {
            auto& server = model.domain.servers.at( 0);
            EXPECT_TRUE( server.alias == "a");
            EXPECT_TRUE(( server.restrictions == std::vector< std::string>{ "a1", "a2", "a3"}));
         }

         {
            auto& server = model.domain.servers.at( 2);
            EXPECT_TRUE( server.alias == "c");
            EXPECT_TRUE(( server.restrictions == std::vector< std::string>{ "c1", "c2"}));
         }
      }

      TEST( configuration_model_transform, transaction)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain:
  name: model
  transaction:
    resources: 
       - name: r1
       - name: r2
  
  groups:
    - name: a
      resources: [ r1]
    - name: b
      resources: [ r2]
  servers:
    - path: a
    - path: b
      memberships: [ a]
    - path: c
      memberships: [ b]
    - path: d
      memberships: [ a, b]
    - path: e
      resources: [ r1]
)";
         
         auto model = local::configuration( configuration);

         EXPECT_TRUE( model.domain.name == "model");
         EXPECT_TRUE( model.domain.servers.size()  == 5);
         
         ASSERT_TRUE( model.transaction.resources.size() == 2) << CASUAL_NAMED_VALUE( model);
         EXPECT_TRUE( model.transaction.resources.at( 0).name == "r1");
         EXPECT_TRUE( model.transaction.resources.at( 1).name == "r2");

         ASSERT_TRUE( model.domain.servers.size() == 5);
         {
            auto& server =  model.domain.servers.at( 1);
            EXPECT_TRUE( server.path == "b");
            EXPECT_TRUE( server.memberships == std::vector< std::string>{ "a"});
         }
         {
            auto& server =  model.domain.servers.at( 2);
            EXPECT_TRUE( server.path == "c");
            EXPECT_TRUE( server.memberships == std::vector< std::string>{ "b"});
         }
         {
            auto& server =  model.domain.servers.at( 3);
            EXPECT_TRUE( server.path == "d");
            EXPECT_TRUE(( server.memberships == std::vector< std::string>{ "a", "b"}));
         }
         {
            auto& server =  model.domain.servers.at( 4);
            EXPECT_TRUE( server.path == "e");
            EXPECT_TRUE( server.memberships.empty());
            EXPECT_TRUE( server.resources == std::vector< std::string>{ "r1"});
         }


      }
   } // configuration

} // casual
