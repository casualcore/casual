//!
//! casual
//!

#include <gtest/gtest.h>

#include "configuration/domain.h"
#include "configuration/example/domain.h"




#include "sf/log.h"
#include "sf/archive/maker.h"


namespace casual
{
   class casual_configuration_domain : public ::testing::TestWithParam< const char*>
   {
   };


   INSTANTIATE_TEST_CASE_P( protocol,
         casual_configuration_domain,
      ::testing::Values(".yaml", ".json", ".xml"));//, ".ini"));


   namespace local
   {
      namespace
      {
         namespace domain
         {
            configuration::domain::Manager init()
            {
               configuration::domain::Manager domain;

               domain.name = "domain1";

               {
                  domain.manager_default.server.instances = 3;
                  domain.manager_default.server.restart = true;
               }

               {
                  domain.manager_default.service.timeout.emplace( "90");
               }

               {
                  domain.transaction.manager_default.resource.instances.emplace( 42);
                  domain.transaction.manager_default.resource.key.emplace( "rm-db2");

                  domain.transaction.resources = {
                        { []( configuration::transaction::Resource& r){
                           r.name = "db1";
                           r.openinfo.emplace( "usr=a,pwd=b");
                        }},
                        { []( configuration::transaction::Resource& r){
                           r.name = "db2";
                           r.openinfo.emplace( "usr=a,pwd=b");
                           r.closeinfo.emplace( "close-info");
                        }},
                  };

               }
               domain.groups = {
                       { []( configuration::group::Group& g){
                          g.name = "group1";
                       }},
                       { []( configuration::group::Group& g){
                           g.name = "group2";
                       }},
               };

               domain.servers = {
                       { []( configuration::server::Server& s){
                          s.alias.emplace( "server1");
                          s.instances = 42;
                       }},
                       { []( configuration::server::Server& s){
                           s.alias.emplace( "server2");
                       }},
               };

               return domain;
            }

            const configuration::domain::Manager& get()
            {
               static auto domain = init();
               return domain;
            }
         } // domain


         common::file::scoped::Path serialize_domain( const std::string& extension)
         {
            return configuration::example::temporary( domain::get(), extension);
         }

      } // <unnamed>
   } // local


	TEST_P( casual_configuration_domain, domain)
	{
	   auto path = local::serialize_domain( GetParam());
	   auto domain = configuration::domain::get( { path.path()});

	   EXPECT_TRUE( domain.name == "domain1") << "name: " << domain.name;
	}

   TEST_P( casual_configuration_domain, groups)
   {
      auto path = local::serialize_domain( GetParam());
      auto domain = configuration::domain::get( { path.path()});

      EXPECT_TRUE( domain.groups == local::domain::get().groups);
   }

	TEST_P( casual_configuration_domain, default_server)
   {
      auto path = local::serialize_domain( GetParam());
      auto domain = configuration::domain::get( { path.path()});

      EXPECT_TRUE( domain.manager_default.server.instances == 3ul) << CASUAL_MAKE_NVP( domain.manager_default.server.instances); //<< CASUAL_MAKE_NVP( path.release());
      EXPECT_TRUE( domain.manager_default.server.restart == true);


   }

   TEST_P( casual_configuration_domain, default_service)
   {
      auto path = local::serialize_domain( GetParam());
      auto domain = configuration::domain::get( { path.path()});

      EXPECT_TRUE( domain.manager_default.service.timeout == std::string( "90"));
   }

   TEST_P( casual_configuration_domain, default_resource)
   {
      auto path = local::serialize_domain( GetParam());
      auto domain = configuration::domain::get( { path.path()});

      EXPECT_TRUE( domain.transaction.manager_default.resource.instances.value() == 42);
      EXPECT_TRUE( domain.transaction.manager_default.resource.key.value() == "rm-db2");
   }


   TEST_P( casual_configuration_domain, transaction)
   {
      auto path = local::serialize_domain( GetParam());
      auto domain = configuration::domain::get( { path.path()});

      ASSERT_TRUE( domain.transaction.resources.size() == 2) << "size: " << domain.transaction.resources.size();
      {
         auto& resource = domain.transaction.resources.at( 0);
         EXPECT_TRUE( resource.instances.value() == 42);
         EXPECT_TRUE( resource.key.value() == "rm-db2");
         EXPECT_TRUE( resource.name == "db1");
         EXPECT_TRUE( resource.openinfo.value() == "usr=a,pwd=b");
      }

      {
         auto& resource = domain.transaction.resources.at( 1);
         EXPECT_TRUE( resource.instances.value() == 42);
         EXPECT_TRUE( resource.key.value() == "rm-db2");
         EXPECT_TRUE( resource.name == "db2");
         EXPECT_TRUE( resource.openinfo.value() == "usr=a,pwd=b");
      }
   }

   TEST_P( casual_configuration_domain, servers)
   {
      auto path = local::serialize_domain( GetParam());
      auto domain = configuration::domain::get( { path.path()});

      ASSERT_TRUE( domain.servers.size() == 2) << "size: " << domain.servers.size();
      EXPECT_TRUE( domain.servers.at( 0).instances == 42ul) << CASUAL_MAKE_NVP( domain.servers) << CASUAL_MAKE_NVP( local::domain::get().servers);
      EXPECT_TRUE( domain.servers == local::domain::get().servers);
   }

   TEST_P( casual_configuration_domain, transaction_manager)
   {
      auto path = local::serialize_domain( GetParam());
      auto domain = configuration::domain::get( { path.path()});

      EXPECT_TRUE( domain.transaction.log == local::domain::get().transaction.log);

   }

} // casual
