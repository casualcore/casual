//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <gtest/gtest.h>
#include "common/unittest.h"

#include "configuration/domain.h"
#include "configuration/example/domain.h"




#include "serviceframework/log.h"
#include "serviceframework/archive/maker.h"


namespace casual
{
   namespace configuration
   {
      class configuration_domain : public ::testing::TestWithParam< const char*>
      {
      };


      INSTANTIATE_TEST_CASE_P( protocol,
            configuration_domain,
         ::testing::Values(".yaml", ".json", ".xml", ".ini"));

      TEST_P( configuration_domain, domain)
      {
         common::unittest::Trace trace;

         // serialize and deserialize
         auto domain = domain::get( { example::temporary( example::domain(), GetParam())});

         EXPECT_TRUE( domain.name == "domain.A42") << "name: " << domain.name;
      }

      TEST_P( configuration_domain, groups)
      {
         common::unittest::Trace trace;

         // serialize and deserialize
         auto domain = domain::get( { example::temporary( example::domain(), GetParam())});

         EXPECT_TRUE( domain.groups.size() == 3) << CASUAL_MAKE_NVP( domain.groups);
      }

      TEST_P( configuration_domain, default_server)
      {
         common::unittest::Trace trace;

         // serialize and deserialize
         auto domain = domain::get( { example::temporary( example::domain(), GetParam())});

         EXPECT_TRUE( domain.manager_default.server.instances.value() == 1) << CASUAL_MAKE_NVP( domain.manager_default.server.instances); //<< CASUAL_MAKE_NVP( path.release());
         EXPECT_TRUE( domain.manager_default.server.restart == true);


      }

      TEST_P( configuration_domain, default_service)
      {
         common::unittest::Trace trace;

         // serialize and deserialize
         auto domain = domain::get( { example::temporary( example::domain(), GetParam())});

         EXPECT_TRUE( domain.manager_default.service.timeout == std::string( "90s"));
      }

      TEST_P( configuration_domain, default_resource)
      {
         common::unittest::Trace trace;

         // serialize and deserialize
         auto domain = domain::get( { example::temporary( example::domain(), GetParam())});

         EXPECT_TRUE( domain.transaction.manager_default.resource.instances.value() == 3);
         EXPECT_TRUE( domain.transaction.manager_default.resource.key.value() == "db2_rm");
      }


      TEST_P( configuration_domain, servers)
      {
         common::unittest::Trace trace;

         // serialize and deserialize
         auto domain = domain::get( { example::temporary( example::domain(), GetParam())});

         ASSERT_TRUE( domain.servers.size() == 5) << "size: " << domain.servers.size();
         EXPECT_TRUE( domain.servers.at( 2).instances.value() == 10) << CASUAL_MAKE_NVP( domain.servers);

      }

      TEST_P( configuration_domain, transaction_manager)
      {
         common::unittest::Trace trace;

         // serialize and deserialize
         auto domain = domain::get( { example::temporary( example::domain(), GetParam())});

         EXPECT_TRUE( domain.transaction.log == "/some/fast/disk/domain.A42/transaction.log");

      }
   } // configuration

} // casual
