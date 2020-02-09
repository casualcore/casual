//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "http/outbound/configuration.h"

#include "common/unittest/file.h"
#include "serviceframework/log.h"

namespace casual
{
   namespace http
   {
      namespace outbound
      {
         TEST( http_outbound_configuration, empty_empty_add__expect_empty)
         {
            common::unittest::Trace trace;

            auto result = configuration::Model{} + configuration::Model{};

            EXPECT_TRUE( result.casual_default.service.headers.empty());
            EXPECT_TRUE( result.casual_default.service.discard_transaction == false);
            EXPECT_TRUE( result.services.empty());
         }

         TEST( http_outbound_configuration, read_basic_configuration_file)
         {
            common::unittest::Trace trace;

            auto file = common::unittest::file::temporary::content( ".yaml", R"(

http:
  services:
    - name: a
      url: a/b.se

)");
            auto model = configuration::get( file);
            EXPECT_TRUE( model.services.size() == 1) << CASUAL_NAMED_VALUE( model);
            EXPECT_TRUE( model.services.at( 0).name == "a");
            EXPECT_TRUE( model.services.at( 0).url == "a/b.se");
         }


         TEST( http_outbound_configuration, default_headers)
         {
            common::unittest::Trace trace;

            auto file = common::unittest::file::temporary::content( ".yaml", R"(
http:
  default:
    service:
      headers:
        - name: xyz
          value: bla
)");
            auto model = configuration::get( file);
            EXPECT_TRUE( model.casual_default.service.headers.size() == 1) << CASUAL_NAMED_VALUE( model);
            EXPECT_TRUE( model.casual_default.service.headers.at( 0).name == "xyz");
            EXPECT_TRUE( model.casual_default.service.headers.at( 0).value == "bla");
         }

         TEST( http_outbound_configuration, default_discard_transaction__true)
         {
            common::unittest::Trace trace;

            auto file = common::unittest::file::temporary::content( ".yaml", R"(

http:
  default:
    service:
       discard_transaction: true
)");
            auto model = configuration::get( file);
            EXPECT_TRUE( model.casual_default.service.discard_transaction == true) << CASUAL_NAMED_VALUE( model);
         }

         TEST( http_outbound_configuration, default_discard_transaction__false)
         {
            common::unittest::Trace trace;

            auto file = common::unittest::file::temporary::content( ".yaml", R"(

http:
  default:
    service:
       discard_transaction: false
)");
            auto model = configuration::get( file);
            EXPECT_TRUE( model.casual_default.service.discard_transaction == false) << CASUAL_NAMED_VALUE( model);
         }


         TEST( http_outbound_configuration, service_discard_transaction__true)
         {
            common::unittest::Trace trace;

            auto file = common::unittest::file::temporary::content( ".yaml", R"(
http:
  services:
    - name: a
      url: a.se/a
      discard_transaction: true
)");
            auto model = configuration::get( file);
            ASSERT_TRUE( model.services.size() == 1);
            EXPECT_TRUE( model.services.at( 0).discard_transaction.value() == true) << CASUAL_NAMED_VALUE( model);
         }

         TEST( http_outbound_configuration, service_discard_transaction__false)
         {
            common::unittest::Trace trace;

            auto file = common::unittest::file::temporary::content( ".yaml", R"(
http:
  services:
    - name: a
      url: a.se/a
      discard_transaction: false
)");
            auto model = configuration::get( file);
            //model.services.at( 0).discard_transaction = true;
            ASSERT_TRUE( model.services.size() == 1) << CASUAL_NAMED_VALUE( model);
            EXPECT_TRUE( model.services.at( 0).discard_transaction.value() == false) << CASUAL_NAMED_VALUE( model);
         }



         TEST( http_outbound_configuration, two_files__expect_aggregate)
         {
            auto file_a = common::unittest::file::temporary::content( ".yaml", R"(

http:
  services:
    - name: a
      url: a.se/a

)");

            auto file_b = common::unittest::file::temporary::content( ".yaml", R"(

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

