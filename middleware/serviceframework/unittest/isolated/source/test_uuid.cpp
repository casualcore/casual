//!
//! test_uuid.cpp
//!
//! Created on: May 5, 2013
//!     Author: Lazan
//!

#include <gtest/gtest.h>

#include "sf/platform.h"
#include "sf/archive/yaml.h"

namespace casual
{
   TEST( casual_sf_uuid, serialize)
   {

      sf::archive::yaml::Save save;
      sf::archive::yaml::Writer writer( save());

      sf::platform::Uuid uuid( sf::platform::uuid::make());

      writer << CASUAL_MAKE_NVP( uuid);

      std::string yaml;
      save( yaml);

      sf::archive::yaml::Load load;
      load( yaml);
      sf::archive::yaml::Reader reader( load());

      sf::platform::Uuid out;
      reader >> sf::makeNameValuePair( "uuid", out);

      EXPECT_TRUE( uuid == out) << "uuid: " << uuid << " out: " << out;
   }

}


