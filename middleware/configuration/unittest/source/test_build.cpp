//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "configuration/example/build/model.h"
#include "configuration/build/model/load.h"
#include "configuration/example/create.h"

#include "common/file.h"

#include "serviceframework/log.h"

namespace casual
{
   namespace configuration
   {
      namespace build
      {
         class configuration_build_server : public ::testing::TestWithParam< const char*>
         {
         };


         INSTANTIATE_TEST_SUITE_P( protocol,
               configuration_build_server,
            ::testing::Values(".yaml", ".json", ".xml", ".ini"));



         TEST_P( configuration_build_server, load_configuration)
         {
            common::unittest::Trace trace;

            // serialize and deserialize
            auto model = build::model::load::server(
               example::create::file::temporary( example::build::model::server(), GetParam()));

            EXPECT_TRUE( model.server.services.size() == 4);
         }

         TEST_P( configuration_build_server, default_service)
         {
            common::unittest::Trace trace;

            // serialize and deserialize
            auto model = build::model::load::server(
               example::create::file::temporary( example::build::model::server(), GetParam()));

            EXPECT_TRUE( model.server.server_default.service.transaction == "join");

            ASSERT_TRUE( model.server.server_default.service.category.has_value());
            EXPECT_TRUE( model.server.server_default.service.category.value() == "some.category");
         }

         TEST_P( configuration_build_server, service_s1__expect_default)
         {
            common::unittest::Trace trace;

            // serialize and deserialize
            auto model = build::model::load::server(
               example::create::file::temporary( example::build::model::server(), GetParam()));

            auto& service = model.server.services.at( 0);

            EXPECT_TRUE( service.name == "s1");
            ASSERT_TRUE( service.transaction) << CASUAL_NAMED_VALUE( service);
            EXPECT_TRUE( service.transaction.value() == "join") << "service.transaction.value(): " << service.transaction.value();
            ASSERT_TRUE( service.function);
            EXPECT_TRUE( service.function.value() == "s1");
            ASSERT_TRUE( service.category);
            EXPECT_TRUE( service.category.value() == "some.category");
         }

         TEST_P( configuration_build_server, service_s2__expect__overridden_transaction)
         {
            common::unittest::Trace trace;

            // serialize and deserialize
            auto model = build::model::load::server(
               example::create::file::temporary( example::build::model::server(), GetParam()));

            auto& service = model.server.services.at( 1);

            EXPECT_TRUE( service.name == "s2");
            EXPECT_TRUE( service.transaction.value() == "auto") << CASUAL_NAMED_VALUE( service.transaction);
         }

         TEST_P( configuration_build_server, service_s3__expect__specified_function)
         {
            common::unittest::Trace trace;

            // serialize and deserialize
            auto model = build::model::load::server(
               example::create::file::temporary( example::build::model::server(), GetParam()));

            auto& service = model.server.services.at( 2);

            EXPECT_TRUE( service.name == "s3");
            EXPECT_TRUE( service.function.value() == "f3");
         }

         TEST_P( configuration_build_server, service_s3__expect__override_category)
         {
            common::unittest::Trace trace;

            // serialize and deserialize
            auto model = build::model::load::server(
               example::create::file::temporary( example::build::model::server(), GetParam()));

            auto& service = model.server.services.at( 3);

            EXPECT_TRUE( service.name == "s4");
            EXPECT_TRUE( service.category.value() == "some.other.category");
         }



      } // resource
   } // configuration
} // casual
