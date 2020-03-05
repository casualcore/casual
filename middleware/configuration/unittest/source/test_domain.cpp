//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "configuration/domain.h"
#include "configuration/example/domain.h"


#include "common/serialize/line.h"
#include "common/serialize/create.h"
#include "common/serialize/native/binary.h"


namespace casual
{
   namespace configuration
   {

      TEST( configuration_domain, domain_binary_serialize)
      {
         common::unittest::Trace trace;

         auto buffer = []()
         {
            auto archive = common::serialize::native::binary::writer();
            domain::Manager domain;
            archive << CASUAL_NAMED_VALUE( domain);
            return archive.consume();
         }();

         
         EXPECT_NO_THROW({
            auto archive = common::serialize::native::binary::reader( buffer);
            domain::Manager domain;
            archive >> CASUAL_NAMED_VALUE( domain);
         });
      }



      class configuration_domain : public ::testing::TestWithParam< const char*>
      {
      };


      INSTANTIATE_TEST_CASE_P( protocol,
            configuration_domain,
         ::testing::Values(".yaml", ".json", ".xml", ".ini"));

      TEST_P( configuration_domain, domain_default_ctor_serialize)
      {
         common::unittest::Trace trace;
         
         // remove '.'
         auto param = GetParam() + 1;

         domain::Manager domain;
         auto output = common::serialize::create::writer::from( param);
         output << CASUAL_NAMED_VALUE( domain);
         
         EXPECT_NO_THROW({
            auto buffer = output.consume< std::stringstream>();
            auto input = common::serialize::create::reader::relaxed::from( param, buffer);
            input >> CASUAL_NAMED_VALUE( domain);
         });
      }

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

         EXPECT_TRUE( domain.groups.size() == 3) << CASUAL_NAMED_VALUE( domain.groups);
      }

      TEST_P( configuration_domain, default_server)
      {
         common::unittest::Trace trace;

         // serialize and deserialize
         auto domain = domain::get( { example::temporary( example::domain(), GetParam())});

         EXPECT_TRUE( domain.manager_default.server.instances == 1) << CASUAL_NAMED_VALUE( domain.manager_default.server.instances); //<< CASUAL_NAMED_VALUE( path.release());
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

         EXPECT_TRUE( domain.transaction.manager_default.resource.instances == 3);
         EXPECT_TRUE( domain.transaction.manager_default.resource.key == "db2_rm");
      }


      TEST_P( configuration_domain, servers)
      {
         common::unittest::Trace trace;

         // serialize and deserialize
         auto domain = domain::get( { example::temporary( example::domain(), GetParam())});

         ASSERT_TRUE( domain.servers.size() == 5) << "size: " << domain.servers.size();
         EXPECT_TRUE( domain.servers.at( 2).instances.value() == 10) << CASUAL_NAMED_VALUE( domain.servers);

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
