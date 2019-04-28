//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include <gtest/gtest.h>
#include "common/unittest.h"
#include "common/unittest/file.h"

#include "configuration/domain.h"
#include "common/exception/system.h"

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

               value.manager_default.environment.variables.push_back( { "domain-a", "value-a"});
               value.manager_default.environment.files.emplace_back( "file-a");

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

               value.manager_default.environment.variables.push_back( { "domain-b", "value-b"});
               value.manager_default.environment.variables.push_back( { "domain-a", "value-a-2"}); // will override from a
               value.manager_default.environment.files.emplace_back( "file-b");

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

         EXPECT_TRUE( value.manager_default.server.instances == 1);
         EXPECT_TRUE( value.manager_default.executable.instances == 1);
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

         auto file_a = common::unittest::file::temporary::content( ".yaml", R"(
domain:
   name: domain-a
   transaction:
      log: a
      resources:
         - name: rm-a
           key: key-a
           instances: 4

         )");

         auto file_b = common::unittest::file::temporary::content( ".yaml", R"(
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

      TEST( configuration_domain_accumulate, default_environment)
      {
         common::unittest::Trace trace;

         auto value =  local::domain_a() + local::domain_b();

         auto& environment = value.manager_default.environment;

         ASSERT_TRUE( environment.variables.size() == 2);
         EXPECT_TRUE( environment.variables[ 0].key == "domain-a");
         EXPECT_TRUE( environment.variables[ 0].value == "value-a-2"); // overridden

         EXPECT_TRUE( environment.variables[ 1].key == "domain-b");
         EXPECT_TRUE( environment.variables[ 1].value == "value-b");

         ASSERT_TRUE( environment.files.size() == 2);
         ASSERT_TRUE( environment.files[ 0] == "file-a");
         ASSERT_TRUE( environment.files[ 1] == "file-b");
      }

      TEST( configuration_domain_accumulate, group_duplicates___throws)
      {
         common::unittest::Trace trace;

         auto file_a = common::unittest::file::temporary::content( ".yaml", R"(
domain:
   name: domain-a
   groups:
     - name: A1
     - name: A2
         )");

         auto file_b = common::unittest::file::temporary::content( ".yaml", R"(
domain:
   name: domain-b
   groups:
     - name: B1
     - name: B2
     - name: A2
         )");

         EXPECT_THROW({
            auto value = domain::get( { file_a.path(), file_b.path()});
         }, common::exception::system::invalid::Argument);
      }

      TEST( configuration_domain_accumulate, group_duplicates_in_one_file__throws)
      {
         common::unittest::Trace trace;

         auto file_a = common::unittest::file::temporary::content( ".yaml", R"(
domain:
   name: domain-a
   groups:
     - name: A1
     - name: A2
         )");

         auto file_b = common::unittest::file::temporary::content( ".yaml", R"(
domain:
   name: domain-b
   groups:
     - name: B1
     - name: B2
     - name: B2
         )");

         EXPECT_THROW({
            auto value = domain::get( { file_a.path(), file_b.path()});
         }, common::exception::system::invalid::Argument);
      }

      TEST( configuration_domain_accumulate, groups)
      {
         common::unittest::Trace trace;

         auto file_a = common::unittest::file::temporary::content( ".yaml", R"(
domain:
   name: domain-a
   groups:
     - name: A1
     - name: A2
         )");

         auto file_b = common::unittest::file::temporary::content( ".yaml", R"(
domain:
   name: domain-b
   groups:
     - name: B1
     - name: B2
         )");

         auto value = domain::get( { file_a.path(), file_b.path()});

         auto& groups = value.groups;
         ASSERT_TRUE( groups.size() == 4);
         EXPECT_TRUE( groups[ 0].name == "A1");
         EXPECT_TRUE( groups[ 1].name == "A2");
         EXPECT_TRUE( groups[ 2].name == "B1");
         EXPECT_TRUE( groups[ 3].name == "B2");
      }


      TEST( configuration_domain_accumulate, executables)
      {
         common::unittest::Trace trace;

         auto file_a = common::unittest::file::temporary::content( ".yaml", R"(
domain:
   name: domain-a
   executables:
     - path: A1
     - path: A2
         )");

         auto file_b = common::unittest::file::temporary::content( ".yaml", R"(
domain:
   name: domain-b
   executables:
     - path: B1
     - path: B2
         )");

         auto value = domain::get( { file_a.path(), file_b.path()});

         auto& executables = value.executables;
         ASSERT_TRUE( executables.size() == 4);
         EXPECT_TRUE( executables[ 0].path == "A1");
         EXPECT_TRUE( executables[ 1].path == "A2");
         EXPECT_TRUE( executables[ 2].path == "B1");
         EXPECT_TRUE( executables[ 3].path == "B2");
      }

      TEST( configuration_domain_accumulate, servers)
      {
         common::unittest::Trace trace;

         auto file_a = common::unittest::file::temporary::content( ".yaml", R"(
domain:
   name: domain-a
   servers:
     - path: A1
     - path: A2
         )");

         auto file_b = common::unittest::file::temporary::content( ".yaml", R"(
domain:
   name: domain-b
   servers:
     - path: B1
     - path: B2
         )");

         auto value = domain::get( { file_a.path(), file_b.path()});

         auto& servers = value.servers;
         ASSERT_TRUE( servers.size() == 4);
         EXPECT_TRUE( servers[ 0].path == "A1");
         EXPECT_TRUE( servers[ 1].path == "A2");
         EXPECT_TRUE( servers[ 2].path == "B1");
         EXPECT_TRUE( servers[ 3].path == "B2");
      }

      TEST( configuration_domain_accumulate, servers_duplicate_path__ok)
      {
         common::unittest::Trace trace;

         auto file_a = common::unittest::file::temporary::content( ".yaml", R"(
domain:
   name: domain-a
   servers:
     - path: A1
       alias: a_A1
     - path: A2
       alias: a_A2
         )");

         auto file_b = common::unittest::file::temporary::content( ".yaml", R"(
domain:
   name: domain-b
   servers:
     - path: A1
       alias: b_A1
     - path: A2
       alias: b_A2
         )");

         auto value = domain::get( { file_a.path(), file_b.path()});

         auto& servers = value.servers;
         ASSERT_TRUE( servers.size() == 4);
         EXPECT_TRUE( servers[ 0].path == "A1");
         EXPECT_TRUE( servers[ 1].path == "A2");
         EXPECT_TRUE( servers[ 2].path == "A1");
         EXPECT_TRUE( servers[ 3].path == "A2");
      }


      TEST( configuration_domain_accumulate, servers_duplicate_alias__throws)
      {
         common::unittest::Trace trace;

         auto file_a = common::unittest::file::temporary::content( ".yaml", R"(
domain:
   name: domain-a
   servers:
     - path: A1
       alias: a1
     - path: A2
       alias: a2
         )");

         auto file_b = common::unittest::file::temporary::content( ".yaml", R"(
domain:
   name: domain-b
   servers:
     - path: A1
       alias: a1_2
     - path: A2
       alias: a2
         )");

         EXPECT_THROW({
            auto value = domain::get( { file_a.path(), file_b.path()});
         }, common::exception::system::invalid::Argument);
      }

   } // configuration   
} // casual
