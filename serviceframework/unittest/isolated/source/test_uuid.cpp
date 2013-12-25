//!
//! test_uuid.cpp
//!
//! Created on: May 5, 2013
//!     Author: Lazan
//!

#include <gtest/gtest.h>

#include "sf/platform.h"
#include "sf/archive_yaml.h"


#include <yaml-cpp/yaml.h>

namespace casual
{
   TEST( casual_sf_uuid, serialize)
   {
      YAML::Emitter emitter;
      sf::archive::yaml::Writer writer( emitter);

      sf::platform::Uuid uuid( sf::platform::Uuid::make());

      writer << CASUAL_MAKE_NVP( uuid);

      std::istringstream stream( emitter.c_str());
      sf::archive::yaml::Reader reader( stream);

      sf::platform::Uuid out;
      reader >> sf::makeNameValuePair( "uuid", out);

      EXPECT_TRUE( uuid == out) << " uuid.string(): " << uuid.string() << " out.string(): " << out.string();
   }

}


