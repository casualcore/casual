//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "administration/unittest/cli/command.h"

#include "domain/unittest/manager.h"

#include <regex>

namespace casual
{
   using namespace common;

   namespace administration
   {
      namespace local
      {
         namespace
         {
             namespace configuration
            {
               constexpr auto base = R"(
domain:
   groups: 
      -  name: base
      -  name: queue
         dependencies: [ base]
      -  name: user
         dependencies: [ queue]
   
   servers:
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager"
        memberships: [ base]
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/casual-transaction-manager"
        memberships: [ base]
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/queue/bin/casual-queue-manager"
        memberships: [ queue]
)";
            } // configuration

            template< typename... C>
            auto domain( C&&... configurations)
            {
               return casual::domain::unittest::manager( configuration::base, std::forward< C>( configurations)...);
            }

            template< typename... Cs>
            auto execute( Cs&&... commands)
            {
               return administration::unittest::cli::command::execute( std::forward< Cs>( commands)...);
            }
         } // <unnamed>
      } // local

      TEST( cli_queue, attributes)
      {
         common::unittest::Trace trace;

         // reply
         {
            auto capture = local::execute( R"(echo "casual" | casual buffer --compose | casual queue --attributes reply b.error | casual pipe --human-sink)");
            EXPECT_TRUE( std::regex_match( capture.standard.out, std::regex{ R"(.*reply: b.error.*\n)"})) << CASUAL_NAMED_VALUE( capture); 
         }

         // properties
         {
            auto capture = local::execute( R"(echo "casual" | casual buffer --compose | casual queue --attributes properties foo | casual pipe --human-sink)");
            EXPECT_TRUE( std::regex_match( capture.standard.out, std::regex{ R"(.*properties: foo,.*\n)"})) << CASUAL_NAMED_VALUE( capture); 
         }

         // available
         {
            auto capture = local::execute( R"(echo "casual" | casual buffer --compose | casual queue --attributes available 42s | casual pipe --human-sink)");
            EXPECT_TRUE( std::regex_match( capture.standard.out, std::regex{ R"(.*available: 42.*\n)"})) << CASUAL_NAMED_VALUE( capture); 
         }
      }

      TEST( cli_queue, enqueue_dequeue)
      {
         common::unittest::Trace trace;


         auto domain = local::domain( R"(
domain:
   queue:
      groups:
         -  alias: Q
            queues:
               -  name: a
               -  name: b

)");
         // enqueue
         {
            auto capture = local::execute( R"(echo "casual" | casual buffer --compose | casual queue --attributes reply b.error | casual queue --enqueue a | casual pipe --human-sink)");
            EXPECT_TRUE( std::regex_match( capture.standard.out, std::regex{ R"([0-9a-f]{32}\n)"})) << CASUAL_NAMED_VALUE( capture);             
         }

         // dequeue
         {
            auto capture = local::execute( R"(casual queue --dequeue a | casual pipe --human-sink)");
            EXPECT_TRUE( std::regex_match( capture.standard.out, std::regex{ R"(.*reply: b.error.*\n)"})) << CASUAL_NAMED_VALUE( capture); 
         }
      }

      TEST( cli_queue, list_forward_groups)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( R"(
domain:
   groups:
      -  name: disabled-group
         dependencies: [ queue]
         enabled: false

   queue:
      forward:
         groups:
            -  alias: forward-group
               memberships:
                  -  disabled-group
               services:
                  -  source: some-queue
                     target:
                        service: some-service
               queues:
                  -  source: some-queue
                     target:
                        queue: some-other-queue
                  -  source: some-other-queue
                     target:
                        queue: yet-another-queue
)");

         // alias
         {
            auto capture = local::execute( R"(casual queue --list-forward-groups --porcelain true | awk -F'|' '{printf $1}')");
            EXPECT_EQ( capture.standard.out, "forward-group") << CASUAL_NAMED_VALUE( capture);
         }

         // services
         {
            auto capture = local::execute( R"(casual queue --list-forward-groups --porcelain true | awk -F'|' '{printf $3}')");
            EXPECT_EQ( capture.standard.out, "1") << CASUAL_NAMED_VALUE( capture);
         }

         // queues
         {
            auto capture = local::execute( R"(casual queue --list-forward-groups --porcelain true | awk -F'|' '{printf $4}')");
            EXPECT_EQ( capture.standard.out, "2") << CASUAL_NAMED_VALUE( capture);
         }

         // enabled
         {
            auto capture = local::execute( R"(casual queue --list-forward-groups --porcelain true | awk -F'|' '{printf $8}')");
            EXPECT_EQ( capture.standard.out, "D") << CASUAL_NAMED_VALUE( capture);
         }
      }

      TEST( cli_queue, list_forward_queues)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( R"(
domain:
   groups:
      -  name: disabled-group
         dependencies: [ queue]
         enabled: false

   queue:
      forward:
         groups:
            -  alias: forward-group
               queues:
                  -  alias: disabled-forward-queue
                     instances: 2
                     memberships:
                        -  disabled-group
                     source: some-queue
                     target:
                        queue: some-other-queue
                        delay: 1s
)");

         // alias
         {
            auto capture = local::execute( R"(casual queue --list-forward-queues --porcelain true | awk -F'|' '{printf $1}')");
            EXPECT_EQ( capture.standard.out, "disabled-forward-queue") << CASUAL_NAMED_VALUE( capture);
         }

         // group
         {
            auto capture = local::execute( R"(casual queue --list-forward-queues --porcelain true | awk -F'|' '{printf $2}')");
            EXPECT_EQ( capture.standard.out, "forward-group") << CASUAL_NAMED_VALUE( capture);
         }

         // source
         {
            auto capture = local::execute( R"(casual queue --list-forward-queues --porcelain true | awk -F'|' '{printf $3}')");
            EXPECT_EQ( capture.standard.out, "some-queue") << CASUAL_NAMED_VALUE( capture);
         }

         // target
         {
            auto capture = local::execute( R"(casual queue --list-forward-queues --porcelain true | awk -F'|' '{printf $4}')");
            EXPECT_EQ( capture.standard.out, "some-other-queue") << CASUAL_NAMED_VALUE( capture);
         }

         // delay
         {
            auto capture = local::execute( R"(casual queue --list-forward-queues --porcelain true | awk -F'|' '{printf $5}')");
            EXPECT_EQ( capture.standard.out, "1.000") << CASUAL_NAMED_VALUE( capture);
         }

         // configured instances
         {
            auto capture = local::execute( R"(casual queue --list-forward-queues --porcelain true | awk -F'|' '{printf $6}')");
            EXPECT_EQ( capture.standard.out, "2") << CASUAL_NAMED_VALUE( capture);
         }

         // instances
         {
            auto capture = local::execute( R"(casual queue --list-forward-queues --porcelain true | awk -F'|' '{printf $7}')");
            EXPECT_EQ( capture.standard.out, "0") << CASUAL_NAMED_VALUE( capture);
         }

         // enabled
         {
            auto capture = local::execute( R"(casual queue --list-forward-queues --porcelain true | awk -F'|' '{printf $11}')");
            EXPECT_EQ( capture.standard.out, "D") << CASUAL_NAMED_VALUE( capture);
         }
      }

      TEST( cli_queue, list_forward_services)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( R"(
domain:
   groups:
      -  name: disabled-group
         dependencies: [ queue]
         enabled: false

   queue:
      forward:
         groups:
            -  alias: forward-group
               services:
                  -  alias: disabled-forward-service
                     instances: 2
                     memberships:
                        -  disabled-group
                     source: some-queue
                     target:
                        service: some-service
                     reply:
                        queue: some-other-queue
                        delay: 1s
)");

         // alias
         {
            auto capture = local::execute( R"(casual queue --list-forward-services --porcelain true | awk -F'|' '{printf $1}')");
            EXPECT_EQ( capture.standard.out, "disabled-forward-service") << CASUAL_NAMED_VALUE( capture);
         }

         // group
         {
            auto capture = local::execute( R"(casual queue --list-forward-services --porcelain true | awk -F'|' '{printf $2}')");
            EXPECT_EQ( capture.standard.out, "forward-group") << CASUAL_NAMED_VALUE( capture);
         }

         // source
         {
            auto capture = local::execute( R"(casual queue --list-forward-services --porcelain true | awk -F'|' '{printf $3}')");
            EXPECT_EQ( capture.standard.out, "some-queue") << CASUAL_NAMED_VALUE( capture);
         }

         // target
         {
            auto capture = local::execute( R"(casual queue --list-forward-services --porcelain true | awk -F'|' '{printf $4}')");
            EXPECT_EQ( capture.standard.out, "some-service") << CASUAL_NAMED_VALUE( capture);
         }

         // reply
         {
            auto capture = local::execute( R"(casual queue --list-forward-services --porcelain true | awk -F'|' '{printf $5}')");
            EXPECT_EQ( capture.standard.out, "some-other-queue") << CASUAL_NAMED_VALUE( capture);
         }

         // delay
         // there is an inconsistency in precision here with forward-queues since the formatter for services returns a string,
         // while the one for queues returns the raw output of std::chrono::duration::count. TODO: which is preferable?
         {
            auto capture = local::execute( R"(casual queue --list-forward-services --porcelain true | awk -F'|' '{printf $6}')");
            EXPECT_EQ( capture.standard.out, "1.000000") << CASUAL_NAMED_VALUE( capture);
         }

         // configured instances
         {
            auto capture = local::execute( R"(casual queue --list-forward-services --porcelain true | awk -F'|' '{printf $7}')");
            EXPECT_EQ( capture.standard.out, "2") << CASUAL_NAMED_VALUE( capture);
         }

         // instances
         {
            auto capture = local::execute( R"(casual queue --list-forward-services --porcelain true | awk -F'|' '{printf $8}')");
            EXPECT_EQ( capture.standard.out, "0") << CASUAL_NAMED_VALUE( capture);
         }

         // enabled
         {
            auto capture = local::execute( R"(casual queue --list-forward-services --porcelain true | awk -F'|' '{printf $12}')");
            EXPECT_EQ( capture.standard.out, "D") << CASUAL_NAMED_VALUE( capture);
         }
      }

   } // administration
} // casual