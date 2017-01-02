//!
//! casual
//!

#include <gtest/gtest.h>
#include "configuration/domain.h"

#include "sf/log.h"
#include "sf/archive/maker.h"


namespace casual
{
   class casual_configuration_domain : public ::testing::TestWithParam< const char*>
   {
   };


   INSTANTIATE_TEST_CASE_P( protocol,
         casual_configuration_domain,
      ::testing::Values(".yaml", ".json", ".xml", ".ini"));


   namespace local
   {
      namespace
      {
         namespace domain
         {
            configuration::domain::Domain init()
            {
               configuration::domain::Domain domain;

               domain.name = "domain1";

               {
                  //domain.casual_default.server.instances = 3;
                  domain.casual_default.server.restart = true;
               }

               {
                  domain.casual_default.service.timeout = "90";
               }


               domain.groups = {
                       { []( configuration::domain::Group& g){
                          g.name = "group1";
                       }},
                       { []( configuration::domain::Group& g){
                           g.name = "group2";
                       }},
               };

               domain.servers = {
                       { []( configuration::domain::Server& s){
                          s.alias = "server1";
                          s.instances = 42;
                       }},
                       { []( configuration::domain::Server& s){
                           s.alias = "server2";
                       }},
               };

               return domain;
            }

            const configuration::domain::Domain& get()
            {
               static auto domain = init();
               return domain;
            }
         } // domain


         common::file::scoped::Path serialize_domain( const std::string& extension)
         {
            common::file::scoped::Path file{ common::file::name::unique( common::directory::temporary() + "/domain", extension)};

            auto& domain = domain::get();

            auto archive = sf::archive::writer::from::file( file);
            archive << CASUAL_MAKE_NVP( domain);

            return file;
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

      EXPECT_TRUE( domain.casual_default.server.instances == 3ul) << CASUAL_MAKE_NVP( domain.casual_default.server.instances);// << CASUAL_MAKE_NVP( path.release());
      EXPECT_TRUE( domain.casual_default.server.restart == true);


   }

   TEST_P( casual_configuration_domain, default_service)
   {
      auto path = local::serialize_domain( GetParam());
      auto domain = configuration::domain::get( { path.path()});

      EXPECT_TRUE( domain.casual_default.service.timeout == "90");
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

      EXPECT_TRUE( domain.transaction.manager.database == local::domain::get().transaction.manager.database);

   }

} // casual
