//!
//! casual
//!

#include <gtest/gtest.h>

#include "http/outbound/configuration.h"


#include "common/mockup/file.h"
#include "sf/log.h"

namespace casual
{
   namespace http
   {
      namespace outbound
      {
         TEST( http_outbound_configuration, empty_empty_add__expect_empty)
         {
            auto result = configuration::Model{} + configuration::Model{};

            EXPECT_TRUE( result.casual_default.headers.empty());
            EXPECT_TRUE( result.services.empty());
         }

         TEST( http_outbound_configuration, read_basic_configuration_file)
         {
            auto file = common::mockup::file::temporary::content( ".yaml", R"(

http:
  services:
    - name: a
      url: a/b.se

)");
            auto model = configuration::get( file);
            EXPECT_TRUE( model.services.size() == 1) << CASUAL_MAKE_NVP( model);
            EXPECT_TRUE( model.services.at( 0).name == "a");
            EXPECT_TRUE( model.services.at( 0).url == "a/b.se");
         }


         TEST( http_outbound_configuration, two_files__expect_aggregate)
         {
            auto file_a = common::mockup::file::temporary::content( ".yaml", R"(

http:
  services:
    - name: a
      url: a.se/a

)");

            auto file_b = common::mockup::file::temporary::content( ".yaml", R"(

http:
  services:
    - name: b
      url: b.se/b

)");
            auto model = configuration::get( std::vector< std::string>{ file_a.path(), file_b.path()});
            EXPECT_TRUE( model.services.size() == 2);
            EXPECT_TRUE( model.services.at( 0).name == "a");
            EXPECT_TRUE( model.services.at( 0).url == "a.se/a");
            EXPECT_TRUE( model.services.at( 1).name == "b");
            EXPECT_TRUE( model.services.at( 1).url == "b.se/b");
         }

      } // outbound
   } // http
} // casual

