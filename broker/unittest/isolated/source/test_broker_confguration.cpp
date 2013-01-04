//!
//! casual_isolatedunittest_broker_confguration.cpp
//!
//! Created on: Nov 5, 2012
//!     Author: Lazan
//!



#include <gtest/gtest.h>

#include "broker/configuration.h"

#include "sf/archive_yaml_implementation.h"


namespace casual
{
   namespace local
   {
      namespace yaml
      {

         std::string getDefault()
         {
            return R"(
broker:
   default:

      server:
         instances: 2
         limits:
            min: 2
            max: 4 

      service:
         timeout: 90

   servers:
      - path: /a/b/c
        instances: 30
        limits:
           max: 40

      - path: /a/b/x
        instances: 10
        limits:
           min: 1
           max: 2

)";

         }

      }

   }

   namespace broker
   {
      TEST( casual_broker_configuration_yaml, read_defaul)
      {
         std::istringstream stream( local::yaml::getDefault());

         sf::archive::yaml::reader::Relaxed reader( stream);

         configuration::Settings broker;

         reader >> CASUAL_MAKE_NVP( broker);

         EXPECT_TRUE( broker.casual_default.server.instances == 2);
         EXPECT_TRUE( broker.casual_default.server.limits.min == 2);
         EXPECT_TRUE( broker.casual_default.server.limits.max == 4);
         EXPECT_TRUE( broker.casual_default.service.timeout == 90);

      }


      TEST( casual_broker_configuration_yaml, read_servers)
      {
         std::istringstream stream( local::yaml::getDefault());

         sf::archive::yaml::reader::Relaxed reader( stream);

         configuration::Settings broker;

         reader >> CASUAL_MAKE_NVP( broker);

         ASSERT_TRUE( broker.servers.size() == 2);
         EXPECT_TRUE( broker.servers.front().path == "/a/b/c");
         EXPECT_TRUE( broker.servers.front().instances == 30);
         EXPECT_TRUE( broker.servers.front().limits.max == 40);

         EXPECT_TRUE( broker.servers.back().path == "/a/b/x");
         EXPECT_TRUE( broker.servers.back().instances == 10);
         EXPECT_TRUE( broker.servers.back().limits.min == 1);
         EXPECT_TRUE( broker.servers.back().limits.max == 2);

      }

   }
}




