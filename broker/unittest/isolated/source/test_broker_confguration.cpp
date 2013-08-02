//!
//! casual_isolatedunittest_broker_confguration.cpp
//!
//! Created on: Nov 5, 2012
//!     Author: Lazan
//!



#include <gtest/gtest.h>

#include "broker/configuration.h"

#include "sf/archive_yaml.h"
#include "sf/archive_logger.h"

#include <fstream>


namespace casual
{
   namespace broker
   {

      /* does not work with g++, no move constructor it seems...
      std::ifstream configurationStream()
      {
         return std::move( std::ifstream( "template_config.yaml"));
      }
      */

      std::istream& configurationStream()
      {
         static std::ifstream stream( "template_config.yaml");
         stream.clear();
         stream.seekg( 0, stream.beg);
         return stream;
      }

      TEST( casual_broker_configuration_yaml, read_defaul)
      {
         //std::istringstream stream( local::yaml::getDefault());
         auto& stream = configurationStream();

         sf::archive::yaml::relaxed::Reader reader( stream);

         configuration::Settings broker;

         reader >> CASUAL_MAKE_NVP( broker);

         sf::archive::logger::Writer debug;

         debug << CASUAL_MAKE_NVP( broker);

         ASSERT_TRUE( broker.servers.size() == 3) << "size: " << broker.servers.size();
         EXPECT_TRUE( broker.casual_default.server.instances == "2");
         EXPECT_TRUE( broker.casual_default.service.timeout == "90");

      }


      TEST( casual_broker_configuration_yaml, read_servers)
      {
         //std::istringstream stream( local::yaml::getDefault());
         auto& stream = configurationStream();

         sf::archive::yaml::relaxed::Reader reader( stream);

         configuration::Settings broker;

         reader >> CASUAL_MAKE_NVP( broker);

         ASSERT_TRUE( broker.servers.size() == 3) << "size: " << broker.servers.size();
         EXPECT_TRUE( broker.servers.at( 0).instances == "1");

         EXPECT_TRUE( broker.servers.at( 2).instances == "10");
         EXPECT_TRUE( broker.servers.at( 2).membership.size() == 2);

      }

   }
}




