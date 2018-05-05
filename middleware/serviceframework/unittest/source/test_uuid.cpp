//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <gtest/gtest.h>

#include "serviceframework/platform.h"
#include "serviceframework/archive/yaml.h"

namespace casual
{
   TEST( casual_sf_uuid, serialize)
   {
      std::string yaml;
      auto uuid = serviceframework::platform::uuid::make();

      {
         auto writer = serviceframework::archive::yaml::writer( yaml);
         writer << CASUAL_MAKE_NVP( uuid);
      }
      auto reader = serviceframework::archive::yaml::reader( yaml);
      serviceframework::platform::Uuid out;
      reader >> serviceframework::name::value::pair::make( "uuid", out);

      EXPECT_TRUE( uuid == out) << "uuid: " << uuid << " out: " << out;
   }

}


