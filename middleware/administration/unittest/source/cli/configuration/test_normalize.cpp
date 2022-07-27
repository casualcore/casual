//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"
#include "common/unittest/file.h"

#include "administration/unittest/cli/command.h"

namespace casual::administration
{
   namespace
   {
      namespace configuration
      {
         namespace single
         {
            constexpr auto config = R"(
system: {}
domain:
  name: some_domain
  servers:
  - path: /some/server
    arguments: []
    alias: server1
    instances: 1
    memberships:
    - some-group
    environment:
      variables:
      - key: SOME_ENV
        value: 12345
)";
         }

         namespace multiple
         {
            constexpr auto config_domain = R"(
domain:
  name: multi_file_domain
  default:
    server:
      instances: 1
      restart: false
    service:
      timeout: 90s
  groups:
  - name: casual-queue
    note: casual-queue resource group
)";
            constexpr auto config_resource_db = R"(
system:
  resources:
  - key: some_db_xa_resource
    server: /some/db_resource
    xa_struct_name: whatever
    libraries:
    - db_lib1
    - db_lib2
    paths:
      library: [/db/lib]
)";
            constexpr auto config_resource_qm = R"(
system:
  resources:
  - key: some_qm_xa_resource
    server: /some/qm_resource
    xa_struct_name: whatever
    libraries:
    - qm_lib1
    - qm_lib2
    paths:
      library: [/qm/lib]
)";
            constexpr auto config_domain_a_lot_of_stuff = R"(
domain:
  environment:
    variables:
    - key: ENV_1
      value: value_1
    - key: ENV_2
      value: value_2
  default:
    server:
      instances: 1
      restart: false
    service:
      execution:
        timeout:
          duration: 90s
  gateway:
    outbound:
      groups:
      - alias: queues_outbound
        connections:
        - address: localhost:1346
  groups:
  - name: casual-queue
    note: casual-queue resource group
  - name: some-group
    note: some group with some services
  servers:
  - path: /some/server/1
    instances: 0
    arguments: [ --configuration, some.yaml ]
)";
            constexpr auto config_domain_servers = R"(
domain:
  servers:
  - path: /some/server
    arguments: []
    alias: server1
    instances: 1
    memberships:
    - some-group
    environment:
      variables:
      - key: SOME_ENV
        value: 12345
  - path: /some/server
    arguments: []
    alias: server2
    instances: 1
    memberships:
    - some-group
    environment:
      variables:
      - key: SOME_ENV
        value: 54321
)";
            constexpr auto config_queues_a = R"(
domain:
  queue:
    groups:
    - alias: queue_group_1
      queues:
      - name: queue_group_1.queue_1
        retry:
          count: 60
          delay: 120
)";
            constexpr auto config_queues_b = R"(
domain:
  queue:
    groups:
    - alias: queue_group_2
      queues:
      - name: queue_group_2.queue_1
        retry:
          count: 25
      - name: queue_group_2.queue_2
        retry:
          count: 25
    - alias: queue_group_3
      queues:
      - name: queue_group_3.queue_1
        retry:
          count: 10
          delay: 60
)";
            constexpr auto config_gateway_inbound = R"(
domain:
  gateway:
    inbound:
      groups:
      - alias: batch_1
        connections:
        - address: localhost:2344
)";
            constexpr auto config_gateway_outbound = R"(
domain:
  gateway:
    outbound:
      groups:
      - alias: outbound_group
        connections:
        - address: localhost:6424
)";
            constexpr auto config_forwards = R"(
domain:
  queue:
    forward:
      groups:
      - alias: queue_group_2
        services:
        - source: queue_group_2.queue_1
          instances: 1
          target:
            service: queue_1
)";
         }
      }
   }

   TEST( cli_configuration_normalize, single_file)
   {
      auto file_base = common::unittest::file::temporary::content( ".yaml", configuration::single::config);


      // Normalize
      auto normalize_output = administration::unittest::cli::command::execute( "casual configuration --normalize " + file_base.string()).string();
      EXPECT_NE( normalize_output, "") << "normalize_output: \"" << normalize_output << ", expected some content";


      // Validate
      auto file_normalized = common::unittest::file::temporary::content( ".yaml", normalize_output);
      testing::internal::CaptureStderr();
      administration::unittest::cli::command::execute("casual configuration --validate " + file_normalized.string());
      auto errors = testing::internal::GetCapturedStderr();
      ASSERT_TRUE(errors.empty()) << "Errors were printed on validation: " << errors << "\nfor normalized config: \n" << normalize_output;
   }

   TEST( cli_configuration_normalize, multiple_files)
   {
      auto file_config_domain =
            common::unittest::file::temporary::content(".yaml", configuration::multiple::config_domain);
      auto file_config_resource_db =
            common::unittest::file::temporary::content(".yaml", configuration::multiple::config_resource_db);
      auto file_config_resource_qm =
            common::unittest::file::temporary::content(".yaml", configuration::multiple::config_resource_qm);
      auto file_config_domain_a_lot_of_stuff =
            common::unittest::file::temporary::content(".yaml", configuration::multiple::config_domain_a_lot_of_stuff);
      auto file_config_domain_servers =
            common::unittest::file::temporary::content(".yaml", configuration::multiple::config_domain_servers);
      auto file_config_queues_a =
            common::unittest::file::temporary::content(".yaml", configuration::multiple::config_queues_a);
      auto file_config_queues_b =
            common::unittest::file::temporary::content(".yaml", configuration::multiple::config_queues_b);
      auto file_config_gateway_inbound =
            common::unittest::file::temporary::content(".yaml", configuration::multiple::config_gateway_inbound);
      auto file_config_gateway_outbound =
            common::unittest::file::temporary::content(".yaml", configuration::multiple::config_gateway_outbound);
      auto file_config_forwards =
            common::unittest::file::temporary::content(".yaml", configuration::multiple::config_forwards);


      // Normalize
      auto command = "casual configuration --normalize " + file_config_domain.string()
                     + ' ' + file_config_resource_db.string()
                     + ' ' + file_config_resource_qm.string()
                     + ' ' + file_config_domain_a_lot_of_stuff.string()
                     + ' ' + file_config_domain_servers.string()
                     + ' ' + file_config_queues_a.string()
                     + ' ' + file_config_queues_b.string()
                     + ' ' + file_config_gateway_inbound.string()
                     + ' ' + file_config_gateway_outbound.string()
                     + ' ' + file_config_forwards.string();
      auto normalize_output = administration::unittest::cli::command::execute( command).string();
      EXPECT_NE( normalize_output, "") << "normalize_output: \"" << normalize_output << ", expected some content";


      // Validate
      auto file_normalized = common::unittest::file::temporary::content( ".yaml", normalize_output);
      testing::internal::CaptureStderr();
      administration::unittest::cli::command::execute("casual configuration --validate " + file_normalized.string());
      auto errors = testing::internal::GetCapturedStderr();
      ASSERT_TRUE(errors.empty()) << "Errors were printed on validation: " << errors << "\nfor normalized config: \n" << normalize_output;
   }
}
