//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"
#include "common/unittest/file.h"

#include "configuration/user.h"
#include "configuration/user/load.h"

#include <optional>

namespace casual
{
   namespace configuration
   {
      namespace user
      {
         namespace local
         {
            namespace
            {
               template< typename T>
               auto& instansiate_optional( std::optional< T>& optional)
               {
                  if( ! optional)
                     optional = T{};
                  return optional.value();
               };


               auto load_configuration = []( auto content)
               {
                  auto path = common::unittest::file::temporary::content( ".yaml", std::move( content));
                  return configuration::user::load( { path});
               };
 

               auto domain_a = []()
               {
                  constexpr auto configuration = R"(
domain:
  name: domain-a
  environment:
    variables:
      - key: domain-a
        value: value-a
    files:
      - file-a
  transaction:
     default:
        resource:
           instances: 2
           key: rm-key-a
     log: a
     resources:
       - name: rm-domain-a
         openinfo: openinfo-a
         closeinfo: closeinfo-a
)";
                  
                  return local::load_configuration( configuration);
               };

               auto domain_b = []()
               {
                  constexpr auto configuration = R"(
domain:
  name: domain-b

  environment:
    variables:
      - key: domain-b
        value: value-b
      - key: domain-a
        value: value-a-2
    files:
      - file-b

  transaction:
     log: b
     default:
        resource:
           instances: 4
           key: rm-key-b
     
     resources:
       - name: rm-domain-b
         openinfo: openinfo-b
         closeinfo: closeinfo-b
       - name: rm-domain-b-2
         instances: 3
         openinfo: openinfo-b-2
         closeinfo: closeinfo-b-2
       - name: rm-domain-a
         instances: 7
         openinfo: openinfo-b-3
         closeinfo: closeinfo-b-3
)";

                  return local::load_configuration( configuration);
               };
            } // <unnamed>
         } // local
         TEST( configuration_domain_accumulate, default_ctor)
         {
            common::unittest::Trace trace;

            auto value = Domain{} + Domain{};

            EXPECT_TRUE( ! value.defaults.has_value());
            EXPECT_TRUE( value.groups.empty());
            EXPECT_TRUE( value.servers.empty());
            EXPECT_TRUE( value.executables.empty());
            EXPECT_TRUE( value.services.empty());
         }

         TEST( configuration_domain_accumulate, transaction)
         {
            common::unittest::Trace trace;

            auto value =  local::domain_a() + local::domain_b();

            auto& tran = value.transaction.value();
            EXPECT_TRUE( tran.log == "b");
            ASSERT_TRUE( tran.resources.size() == 3);
            {
               auto& resource = tran.resources[ 0];
               EXPECT_TRUE( resource.name == "rm-domain-b");
               EXPECT_TRUE( resource.key.value() == "rm-key-b");
               EXPECT_TRUE( resource.instances.value() == 4);
            }

            {
               auto& resource = tran.resources[ 1];
               EXPECT_TRUE( resource.name == "rm-domain-b-2");
               EXPECT_TRUE( resource.key.value() == "rm-key-b");
               EXPECT_TRUE( resource.instances.value() == 3);
            }
            {
               auto& resource = tran.resources[ 2];
               // replaced by b-file
               EXPECT_TRUE( resource.name == "rm-domain-a") << CASUAL_NAMED_VALUE( tran.resources);
               EXPECT_TRUE( resource.key.value() == "rm-key-b");
               EXPECT_TRUE( resource.instances.value() == 7);
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

            auto value = user::load( { file_a, file_b});

            ASSERT_TRUE( value.transaction);
            auto& tran = value.transaction.value();
            
            EXPECT_TRUE( tran.log == "b");
            ASSERT_TRUE( tran.resources.size() == 2) << CASUAL_NAMED_VALUE( tran);
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

            auto& environment = local::instansiate_optional( value.environment);

            ASSERT_TRUE( environment.variables);
            ASSERT_TRUE( environment.variables.value().size() == 2);
            EXPECT_TRUE( environment.variables.value()[ 0].key == "domain-b");
            EXPECT_TRUE( environment.variables.value()[ 0].value == "value-b"); 

            EXPECT_TRUE( environment.variables.value()[ 1].key == "domain-a");
            EXPECT_TRUE( environment.variables.value()[ 1].value == "value-a-2"); // overridden

            ASSERT_TRUE( environment.files);
            ASSERT_TRUE( environment.files.value().size() == 2);
            EXPECT_TRUE( environment.files.value()[ 0] == "file-a");
            EXPECT_TRUE( environment.files.value()[ 1] == "file-b");
         }

         TEST( configuration_domain_accumulate, group_duplicates__replaced)
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

            auto value = user::load( { file_a, file_b});
            auto& groups = value.groups;
            ASSERT_TRUE( groups.size() == 4);
            EXPECT_TRUE( groups.at( 0).name == "A1");
            EXPECT_TRUE( groups.at( 1).name == "B1");
            EXPECT_TRUE( groups.at( 2).name == "B2");
            EXPECT_TRUE( groups.at( 3).name == "A2");
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

            EXPECT_CODE({
               auto value = user::load( { file_a, file_b});
            }, common::code::casual::invalid_configuration);
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

            auto value = user::load( { file_a, file_b});

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

            auto value = user::load( { file_a, file_b});

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

            auto value = user::load( { file_a, file_b});

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

            auto value = user::load( { file_a, file_b});

            auto& servers = value.servers;
            ASSERT_TRUE( servers.size() == 4);
            EXPECT_TRUE( servers[ 0].path == "A1");
            EXPECT_TRUE( servers[ 1].path == "A2");
            EXPECT_TRUE( servers[ 2].path == "A1");
            EXPECT_TRUE( servers[ 3].path == "A2");
         }


         TEST( configuration_domain_accumulate, servers_duplicate_alias_within_one_file__throws)
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
      - path: XXX
        alias: a1
)");

            EXPECT_CODE({
               auto value = user::load( { file_a});
            }, common::code::casual::invalid_configuration );
         }

         TEST( configuration_domain_accumulate, gateway_inbound_default_discovery)
         {
            common::unittest::Trace trace;

            auto file = common::unittest::file::temporary::content( ".yaml", R"(
domain:
   name: foo
   gateway:
      inbound:
         default:
            connection:
               discovery:
                  forward: true

         groups:
            -  connections:
               -  address: localhost:666
               -  address: localhost:667
                  discovery:
                     forward: false
            -  connections:
               -  address: localhost:777
               -  address: localhost:778
                  discovery:
                     forward: false


)");

            
            auto value = user::load( { file});

            {
               auto& connections = value.gateway.value().inbound.value().groups.at( 0).connections;
               EXPECT_TRUE( connections.at( 0).discovery.value().forward == true);
               EXPECT_TRUE( connections.at( 1).discovery.value().forward == false);
            }
            {
               auto& connections = value.gateway.value().inbound.value().groups.at( 1).connections;
               EXPECT_TRUE( connections.at( 0).discovery.value().forward == true);
               EXPECT_TRUE( connections.at( 1).discovery.value().forward == false);
            }
            
         }
      } // user
   } // configuration   
} // casual
