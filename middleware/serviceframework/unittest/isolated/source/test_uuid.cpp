//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <gtest/gtest.h>

#include "sf/platform.h"
#include "sf/archive/yaml.h"

namespace casual
{
   TEST( casual_sf_uuid, serialize)
   {
      std::string yaml;
      auto uuid = sf::platform::uuid::make();

      {
         auto writer = sf::archive::yaml::writer( yaml);
         writer << CASUAL_MAKE_NVP( uuid);
      }
      auto reader = sf::archive::yaml::reader( yaml);
      sf::platform::Uuid out;
      reader >> sf::name::value::pair::make( "uuid", out);

      EXPECT_TRUE( uuid == out) << "uuid: " << uuid << " out: " << out;
   }

}


