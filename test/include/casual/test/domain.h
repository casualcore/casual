//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "domain/manager/unittest/process.h"

#include <string>
#include <vector>

namespace casual
{
   namespace test
   {
      namespace domain
      {
         struct Manager : casual::domain::manager::unittest::Process
         {
            Manager( std::vector< std::string_view> configuration);

            inline Manager( std::string_view configuration) : Manager{ std::vector< std::string_view>{ configuration}} {}

            inline Manager() : Manager{ configuration} {}

            constexpr static auto configuration = R"(
domain:
   name: test-default-domain

   groups: 
      - name: base
      - name: transaction
        dependencies: [ base]
      - name: queue
        dependencies: [ transaction]
      - name: example
        dependencies: [ queue]

   servers:
      - path: ${CASUAL_HOME}/bin/casual-service-manager
        memberships: [ base]
      - path: ${CASUAL_HOME}/bin/casual-transaction-manager
        memberships: [ transaction]
      - path: ${CASUAL_HOME}/bin/casual-queue-manager
        memberships: [ queue]
      - path: ${CASUAL_HOME}/bin/casual-example-error-server
        memberships: [ example]
      - path: ${CASUAL_HOME}/bin/casual-example-server
        memberships: [ example]
)";
         };
      } // domain
   } // test
} // casual