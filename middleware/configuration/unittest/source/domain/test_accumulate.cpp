//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include <gtest/gtest.h>
#include "common/unittest.h"

#include "configuration/domain.h"

#include "common/mockup/file.h"

namespace casual
{
   namespace configuration
   {
      namespace local
      {
         namespace
         {
            auto domain_a = []()
            {
               domain::Manager value;

               // transaction
               {
                  value.transaction.log = "a";
                  value.transaction.manager_default.resource.instances = 2;
                  value.transaction.manager_default.resource.key = std::string( "rm-key-a");
                  value.transaction.resources = {
                     [](){
                        transaction::Resource result;

                        result.name = "rm-domain-a";
                        //result.instances = 2; will be set by default
                        result.openinfo = std::string{ "openinfo-a"};
                        result.closeinfo = std::string{ "closeinfo-a"};

                        return result;
                     }()
                  };
               }
               domain::finalize( value);
               return value;
            };

            auto domain_b = []()
            {
               domain::Manager value;

               // transaction
               {
                  value.transaction.log = "b";
                  value.transaction.manager_default.resource.instances = 4;
                  value.transaction.manager_default.resource.key = std::string( "rm-key-b");
                  value.transaction.resources = {
                     [](){
                        transaction::Resource result;

                        result.name = "rm-domain-b";
                        //result.instances = 4; will be set by default
                        result.openinfo = std::string{ "openinfo-b"};
                        result.closeinfo = std::string{ "closeinfo-b"};

                        return result;
                     }(),
                     [](){
                        transaction::Resource result;

                        result.name = "rm-domain-b-2";
                        result.instances = 3;
                        result.openinfo = std::string{ "openinfo-b-2"};
                        result.closeinfo = std::string{ "closeinfo-b-2"};

                        return result;
                     }(),
                     [](){
                        transaction::Resource result;
                        result.key.emplace( "rm-key-b-3");
                        result.name = "rm-domain-a"; 
                        result.instances = 7;
                        result.openinfo = std::string{ "openinfo-b-3"};
                        result.closeinfo = std::string{ "closeinfo-b-3"};

                        return result;
                     }()
                  };
               }

               domain::finalize( value);
               return value;
            };
         } // <unnamed>
      } // local
      TEST( configuration_domain_accumulate, empty)
      {
         common::unittest::Trace trace;

         auto value = domain::Manager{} + domain::Manager{};

         EXPECT_TRUE( value.manager_default.server.instances.value() == 1);
         EXPECT_TRUE( value.manager_default.executable.instances.value() == 1);
         EXPECT_TRUE( value.groups.empty());
         EXPECT_TRUE( value.servers.empty());
         EXPECT_TRUE( value.executables.empty());
         EXPECT_TRUE( value.services.empty());
      }

      TEST( configuration_domain_accumulate, transaction)
      {
         common::unittest::Trace trace;

         auto value =  local::domain_a() + local::domain_b();

         auto& tran = value.transaction;
         EXPECT_TRUE( tran.log == "b");
         ASSERT_TRUE( tran.resources.size() == 3);
         {
            auto& resource = tran.resources[ 0];
            EXPECT_TRUE( resource.name == "rm-domain-a");
            EXPECT_TRUE( resource.key.value() == "rm-key-b-3"); // overridden from domain_b
            EXPECT_TRUE( resource.instances.value() == 7); // overridden from domain_b
         }
         {
            auto& resource = tran.resources[ 1];
            EXPECT_TRUE( resource.name == "rm-domain-b");
            EXPECT_TRUE( resource.key.value() == "rm-key-b");
            EXPECT_TRUE( resource.instances.value() == 4);
         }

         {
            auto& resource = tran.resources[ 2];
            EXPECT_TRUE( resource.name == "rm-domain-b-2");
            EXPECT_TRUE( resource.key.value() == "rm-key-b");
            EXPECT_TRUE( resource.instances.value() == 3);
         }
      }

      TEST( configuration_domain_accumulate, transaction_files)
      {
         common::unittest::Trace trace;

         auto file_a = common::mockup::file::temporary::content( ".yaml", R"(
domain:
   name: domain-a
   transaction:
      log: a
      resources:
         - name: rm-a
           key: key-a
           instances: 4

         )");

         auto file_b = common::mockup::file::temporary::content( ".yaml", R"(
domain:
   name: domain-b
   transaction:
      log: b
      resources:
         - name: rm-b
           key: key-b
           instances: 2
         )");

         auto value = domain::get( { file_a.path(), file_b.path()});

         auto& tran = value.transaction;
         EXPECT_TRUE( tran.log == "b");
         ASSERT_TRUE( tran.resources.size() == 2);
         {
            auto& resource = tran.resources[ 0];
            EXPECT_TRUE( resource.name == "rm-a");
            EXPECT_TRUE( resource.key.value() == "key-a");
            EXPECT_TRUE( resource.instances.value() == 4);
         }
         {
            auto& resource = tran.resources[ 1];
            EXPECT_TRUE( resource.name == "rm-b");
            EXPECT_TRUE( resource.key.value() == "key-b");
            EXPECT_TRUE( resource.instances.value() == 2);
         }
      }
   } // configuration   
} // casual