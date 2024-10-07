//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "administration/unittest/cli/command.h"

#include "domain/unittest/manager.h"
#include "domain/unittest/discover.h"

#include "gateway/unittest/utility.h"

#include "common/string.h"

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
            // 42s after unix epoch. Since we're using local + utc-offset, the date could be before 1970-01-01
            EXPECT_TRUE( std::regex_match( capture.standard.out, std::regex{ R"(.*available: .*T.*42[.]000000.*\n)"})) << CASUAL_NAMED_VALUE( capture); 
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

      TEST( cli_queue, enqueue_dequeue_ids)
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
         constexpr auto enqueue_command = R"(echo "casual" | casual buffer --compose | casual buffer --duplicate 10 | casual queue --enqueue a | casual pipe --human-sink)";
         // sort the qid's and dequeue them all with help of xargs
         constexpr auto dequeue_command = R"(sort | xargs casual queue --dequeue a | casual queue --enqueue b | casual pipe --human-sink)";
         
         auto capture = local::execute( enqueue_command, '|', dequeue_command, " | wc -l" );

         EXPECT_TRUE( std::stoi( capture.standard.out) == 10) << CASUAL_NAMED_VALUE( capture);

      }

      TEST( cli_queue, list_queues)
      {
         common::unittest::Trace trace;


         auto domain = local::domain( R"(
domain:
   name: A
   queue:
      groups:
         -  alias: A
            queues:
               -  name: a
                  enable:
                     enqueue: false
               -  name: b
                  enable:
                     dequeue: false
               -  name: c
                  enable:
                     enqueue: false
                     dequeue: false
               -  name: d               

)");


         constexpr std::string_view expected = R"(name     group  rc  rd     count  size  avg  E   EQ  DQ  UC  last
-------  -----  --  -----  -----  ----  ---  --  --  --  --  ----
a        A       0  0.000      0     0    0   D   0   0   0  -   
b        A       0  0.000      0     0    0   E   0   0   0  -   
c        A       0  0.000      0     0    0   -   0   0   0  -   
d        A       0  0.000      0     0    0  ED   0   0   0  -   
a.error  A       0  0.000      0     0    0  ED   0   0   0  -   
b.error  A       0  0.000      0     0    0  ED   0   0   0  -   
c.error  A       0  0.000      0     0    0  ED   0   0   0  -   
d.error  A       0  0.000      0     0    0  ED   0   0   0  -   
)";

         auto capture = local::execute( R"(casual --color false queue --list-queues)");

         EXPECT_TRUE( capture.standard.out == expected) << capture.standard.out;
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

      namespace local
      {
         namespace
         {
            constexpr auto gateway = R"(
domain:
   groups: 
      -  name: gateway
         dependencies: [ user]
   
   servers:
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/gateway/bin/casual-gateway-manager"
        memberships: [ gateway]
)";
         } // <unnamed>
      } // local

      TEST( cli_queue, list_queue_instances)
      {
         common::unittest::Trace trace;

         auto b = local::domain( local::gateway, R"(
domain:
   name: B
   queue:
      groups:
         -  alias: GB
            queues:
               -  name: b1
               -  name: b2
   gateway:
      inbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7001
)");

         auto a = local::domain( local::gateway, R"(
domain:
   name: A
   queue:
      groups:
         -  alias: GA
            queues:
               -  name: a1
               -  name: a2
   gateway:
      outbound:
         groups:
            -  alias: out
               connections:
               -  address: 127.0.0.1:7001
)");

         gateway::unittest::fetch::until( gateway::unittest::fetch::predicate::outbound::connected());
         
         casual::domain::unittest::discover( {}, { "b1", "b2"});

/*

Terminal output

queue     state     pid    alias  description
--------  --------  -----  -----  -----------
a1        internal  50231  GA     -          
a1.error  internal  50231  GA     -          
a2        internal  50231  GA     -          
a2.error  internal  50231  GA     -          
b1        external  50233  out    B          
b2        external  50233  out    B 

*/

         {
            auto capture = local::execute( R"(casual --porcelain true queue --list-queue-instances | grep 'internal')");
            auto rows = string::split( capture.standard.out, '\n');

            EXPECT_TRUE( std::regex_match( rows.at( 0), std::regex{ R"(a1\|internal\|[0-9]+\|GA\|)"})) << rows.at( 0);
            EXPECT_TRUE( std::regex_match( rows.at( 1), std::regex{ R"(a1.error\|internal\|[0-9]+\|GA\|)"}));

         }

         {
            auto capture = local::execute( R"(casual --porcelain true queue --list-queue-instances | grep 'external')");
            auto rows = string::split( capture.standard.out, '\n');

            EXPECT_TRUE( std::regex_match( rows.at( 0), std::regex{ R"(b1\|external\|[0-9]+\|out\|B)"})) << rows.at( 0);
            EXPECT_TRUE( std::regex_match( rows.at( 1), std::regex{ R"(b2\|external\|[0-9]+\|out\|B)"}));


         }
      }

   } // administration
} // casual