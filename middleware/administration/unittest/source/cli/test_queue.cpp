//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"
#include "common/unittest/file.h"
#include "common/unittest/environment.h"

#include "administration/unittest/cli/command.h"

#include "domain/unittest/manager.h"
#include "domain/unittest/discover.h"

#include "gateway/unittest/utility.h"

#include "common/string.h"
#include "common/sink.h"

#include "sql/database.h"

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
      - path: "${CMAKE_BINARY_DIR}/middleware/service/bin/casual-service-manager"
        memberships: [ base]
      - path: "${CMAKE_BINARY_DIR}/middleware/transaction/bin/casual-transaction-manager"
        memberships: [ base]
      - path: "${CMAKE_BINARY_DIR}/middleware/queue/bin/casual-queue-manager"
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
            auto capture = local::execute( R"(echo "casual" | casual buffer --compose | casual queue attributes --reply b.error | casual pipe --human-sink)");
            EXPECT_TRUE( capture.standard.out.contains( "reply: b.error")) << CASUAL_NAMED_VALUE( capture); 
         }

         // properties
         {
            auto capture = local::execute( R"(echo "casual" | casual buffer --compose | casual queue attributes --properties foo | casual pipe --human-sink)");
            EXPECT_TRUE( capture.standard.out.contains( "properties: foo,")) << CASUAL_NAMED_VALUE( capture); 
         }

         // header
         {
            auto capture = local::execute( R"(echo "casual" | casual buffer --compose | casual buffer --header a:1 b:2 c:3 | casual pipe --human-sink)");
            EXPECT_TRUE( capture.standard.out.contains( "fields: [a:1, b:2, c:3]")) << CASUAL_NAMED_VALUE( capture); 
         }

         // available
         {
            auto capture = local::execute( R"(echo "casual" | casual buffer --compose | casual queue attributes --available 42s | casual pipe --human-sink)");
            // 42s after unix epoch. Since we're using local + utc-offset, the date could be before 1970-01-01
            EXPECT_TRUE( std::regex_match( capture.standard.out, std::regex{ R"(.*available: .*T.*42[.]000000.*\n)"})) << CASUAL_NAMED_VALUE( capture); 
         }
      }

      TEST( cli_queue, header_propagation)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( R"(
domain:
   name: A
   queue:
      groups:
         -  alias: Q
            queues:
               -  name: a
)");

            auto capture = local::execute( R"(echo "casual" \
                | casual buffer --compose \
                | casual buffer --header a:1 b:2 c:3 \
                | casual queue --enqueue a \
                | casual queue --dequeue a \
                | casual pipe --human-sink)");

            EXPECT_TRUE( capture.standard.out.contains( "fields: [a:1, b:2, c:3]")) << CASUAL_NAMED_VALUE( capture);

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

      TEST( cli_queue, hidden_queues)
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
               -  name: .b
)");

         // .b should be hidden by default
         {
            auto capture = local::execute( R"(casual --porcelain true queue --list-queues)");
            auto rows = string::split( capture.standard.out, '\n');
            EXPECT_TRUE( algorithm::any_of( rows, []( auto& row){ return row.starts_with( "a|");}));
            EXPECT_TRUE( algorithm::none_of( rows, []( auto& row){ return row.starts_with( ".b|");}));
         }

         // .b should be shown with --all
         {
            auto capture = local::execute( R"(casual --porcelain true queue --list-queues --all)");
            auto rows = string::split( capture.standard.out, '\n');
            EXPECT_TRUE( algorithm::any_of( rows, []( auto& row){ return row.starts_with( "a|");}));
            EXPECT_TRUE( algorithm::any_of( rows, []( auto& row){ return row.starts_with( ".b|");}));
         }

         // instances
         {
            auto capture = local::execute( R"(casual --porcelain true queue --list-queue-instances)");
            auto rows = string::split( capture.standard.out, '\n');
            EXPECT_TRUE( algorithm::any_of( rows, []( auto& row){ return row.starts_with( "a|");}));
            EXPECT_TRUE( algorithm::none_of( rows, []( auto& row){ return row.starts_with( ".b|");}));
         }

         {
            auto capture = local::execute( R"(casual --porcelain true queue --list-queue-instances --all)");
            auto rows = string::split( capture.standard.out, '\n');
            EXPECT_TRUE( algorithm::any_of( rows, []( auto& row){ return row.starts_with( "a|");}));
            EXPECT_TRUE( algorithm::any_of( rows, []( auto& row){ return row.starts_with( ".b|");}));
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
            auto capture = local::execute( R"(casual queue forward --list-queues --porcelain true | awk -F'|' '{printf $1}')");
            EXPECT_EQ( capture.standard.out, "disabled-forward-queue") << CASUAL_NAMED_VALUE( capture);
         }

         // group
         {
            auto capture = local::execute( R"(casual queue forward --list-queues --porcelain true | awk -F'|' '{printf $2}')");
            EXPECT_EQ( capture.standard.out, "forward-group") << CASUAL_NAMED_VALUE( capture);
         }

         // source
         {
            auto capture = local::execute( R"(casual queue forward --list-queues --porcelain true | awk -F'|' '{printf $3}')");
            EXPECT_EQ( capture.standard.out, "some-queue") << CASUAL_NAMED_VALUE( capture);
         }

         // target
         {
            auto capture = local::execute( R"(casual queue forward --list-queues --porcelain true | awk -F'|' '{printf $4}')");
            EXPECT_EQ( capture.standard.out, "some-other-queue") << CASUAL_NAMED_VALUE( capture);
         }

         // delay
         {
            auto capture = local::execute( R"(casual queue forward --list-queues --porcelain true | awk -F'|' '{printf $5}')");
            EXPECT_EQ( capture.standard.out, "1.000") << CASUAL_NAMED_VALUE( capture);
         }

         // configured instances
         {
            auto capture = local::execute( R"(casual queue forward --list-queues --porcelain true | awk -F'|' '{printf $6}')");
            EXPECT_EQ( capture.standard.out, "2") << CASUAL_NAMED_VALUE( capture);
         }

         // instances
         {
            auto capture = local::execute( R"(casual queue forward --list-queues --porcelain true | awk -F'|' '{printf $7}')");
            EXPECT_EQ( capture.standard.out, "0") << CASUAL_NAMED_VALUE( capture);
         }

         // enabled
         {
            auto capture = local::execute( R"(casual queue forward --list-queues --porcelain true | awk -F'|' '{printf $11}')");
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
            auto capture = local::execute( R"(casual queue forward --list-services --porcelain true | awk -F'|' '{printf $1}')");
            EXPECT_EQ( capture.standard.out, "disabled-forward-service") << CASUAL_NAMED_VALUE( capture);
         }

         // group
         {
            auto capture = local::execute( R"(casual queue forward --list-services --porcelain true | awk -F'|' '{printf $2}')");
            EXPECT_EQ( capture.standard.out, "forward-group") << CASUAL_NAMED_VALUE( capture);
         }

         // source
         {
            auto capture = local::execute( R"(casual queue forward --list-services --porcelain true | awk -F'|' '{printf $3}')");
            EXPECT_EQ( capture.standard.out, "some-queue") << CASUAL_NAMED_VALUE( capture);
         }

         // target
         {
            auto capture = local::execute( R"(casual queue forward --list-services --porcelain true | awk -F'|' '{printf $4}')");
            EXPECT_EQ( capture.standard.out, "some-service") << CASUAL_NAMED_VALUE( capture);
         }

         // reply
         {
            auto capture = local::execute( R"(casual queue forward --list-services --porcelain true | awk -F'|' '{printf $5}')");
            EXPECT_EQ( capture.standard.out, "some-other-queue") << CASUAL_NAMED_VALUE( capture);
         }

         // delay
         // there is an inconsistency in precision here with forward-queues since the formatter for services returns a string,
         // while the one for queues returns the raw output of std::chrono::duration::count. TODO: which is preferable?
         {
            auto capture = local::execute( R"(casual queue forward --list-services --porcelain true | awk -F'|' '{printf $6}')");
            EXPECT_EQ( capture.standard.out, "1.000000") << CASUAL_NAMED_VALUE( capture);
         }

         // configured instances
         {
            auto capture = local::execute( R"(casual queue forward --list-services --porcelain true | awk -F'|' '{printf $7}')");
            EXPECT_EQ( capture.standard.out, "2") << CASUAL_NAMED_VALUE( capture);
         }

         // instances
         {
            auto capture = local::execute( R"(casual queue forward --list-services --porcelain true | awk -F'|' '{printf $8}')");
            EXPECT_EQ( capture.standard.out, "0") << CASUAL_NAMED_VALUE( capture);
         }

         // enabled
         {
            auto capture = local::execute( R"(casual queue forward --list-services --porcelain true | awk -F'|' '{printf $12}')");
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
      - path: "${CMAKE_BINARY_DIR}/middleware/gateway/bin/casual-gateway-manager"
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
         
         casual::domain::unittest::discover::request( {}, { "b1", "b2"});

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
            EXPECT_TRUE( std::regex_match( rows.at( 1), std::regex{ R"(b2\|external\|[0-9]+\|out\|B)"})) << CASUAL_NAMED_VALUE( rows);


         }
      }

      TEST( cli_queue, clear)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( R"(
domain:
   queue:
      groups:
         -  alias: Q
            queues:
               -  name: a
)");

         // enqueue 10 messages
         {
            auto capture = local::execute( R"(echo "casual" | casual buffer --compose | casual buffer --duplicate 10 | casual queue --enqueue a | casual pipe --human-sink | wc -l)");
            EXPECT_TRUE( std::stoi( capture.standard.out) == 10) << CASUAL_NAMED_VALUE( capture);
         }

         // clear
         {
            auto capture = local::execute( R"(casual --porcelain true queue --clear a)");
            constexpr std::string_view expected = R"(a|10
)";
            EXPECT_TRUE( capture.standard.out == expected) << CASUAL_NAMED_VALUE( capture);
         }

         // verify queue is empty
         {
            auto capture = local::execute( R"(casual --porcelain true queue --list-messages a)");
            EXPECT_TRUE( capture.standard.out.empty()) << CASUAL_NAMED_VALUE( capture);
         }
      }

      TEST( cli_queue, remove_messages)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( R"(
domain:
   queue:
      groups:
         -  alias: Q
            queues:
               -  name: a
)");
         {
            constexpr auto enqueue_command = R"(echo "casual" | casual buffer --compose | casual buffer --duplicate 10 | casual queue --enqueue a | casual pipe --human-sink)";
            constexpr auto remove_command = R"(xargs casual --porcelain true queue --remove-messages a)";
            
            auto capture = local::execute( enqueue_command, '|', remove_command, " | wc -l"); 
            EXPECT_TRUE( std::stoi( capture.standard.out) == 10) << CASUAL_NAMED_VALUE( capture);
         }

         // verify queue is empty
         {
            auto capture = local::execute( R"(casual --porcelain true queue --list-messages a | wc -l)");
            EXPECT_TRUE( std::stoi( capture.standard.out) == 0) << CASUAL_NAMED_VALUE( capture);
         }
      }

      TEST( cli_queue, force_remove_messages)
      {
         common::unittest::Trace trace;

         signal::callback::registration< code::signal::child>( [](){});

         constexpr std::string_view configuration = R"(
domain:
   name: A
   queue:
      groups:
         -  alias: Q
            queuebase: ${CASUAL_UNITTEST_QUEUEBASE}
            queues:
               - name: a
)";

         auto directory = common::unittest::directory::temporary::Scoped{};
         auto database_path = ( directory.path() / "q.qb").string();
         auto guard = common::unittest::environment::scoped::variable( "CASUAL_UNITTEST_QUEUEBASE", database_path);

         auto domain = local::domain( configuration);

         // enqueue 10 messages
         {
            auto capture = local::execute( R"(echo "casual" | casual buffer --compose | casual buffer --duplicate 10 | casual queue --enqueue a | casual pipe --human-sink | wc -l)");
            EXPECT_TRUE( std::stoi( capture.standard.out) == 10) << CASUAL_NAMED_VALUE( capture);
         }

         common::sink( std::move( domain));

         // hack messages to state enqueued - not removeable without --force
         {
            auto connection = sql::database::Connection( database_path);
            connection.query( "UPDATE message SET state = 3;").execute();
         }

         domain = local::domain( configuration);

         constexpr auto list_message_ids = R"(casual --porcelain true queue --list-messages a | cut -d '|' -f 1)";
         // try to remove messages without --force - expect none removed
         {
            auto capture = local::execute( list_message_ids, "| xargs casual --porcelain true queue --remove-messages a | wc -l"); 
            EXPECT_TRUE( std::stoi( capture.standard.out) == 0) << CASUAL_NAMED_VALUE( capture);
         }

         // use (the) force - expect all to be removed
         {
            auto capture = local::execute( list_message_ids, "| xargs -I {} casual --porcelain true queue --remove-messages a {} --force | wc -l");
            EXPECT_TRUE( std::stoi( capture.standard.out) == 10) << CASUAL_NAMED_VALUE( capture);
         }

         {
            auto capture = local::execute( R"(casual --porcelain true queue --list-messages a | wc -l)");
            EXPECT_TRUE( std::stoi( capture.standard.out) == 0) << CASUAL_NAMED_VALUE( capture);
         }
      }

      TEST( cli_queue, enable_forward_service_queue_membership__expect_enabled)
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
                     instances: 1
                     memberships:
                        -  disabled-group
                     source: some-queue
                     target:
                        service: some-service
               queues:
                  -  alias: disabled-forward-queue
                     instances: 1
                     memberships:
                        - disabled-group
                     source: some-other-queue
                     target:
                        queue: yet-another-queue
)");

         // forward service
         {
            auto capture = local::execute( R"(casual queue --list-forward-services --porcelain true | awk -F'|' '{printf $12}')");
            EXPECT_EQ( capture.standard.out, "D") << CASUAL_NAMED_VALUE( capture);
         }

         // forward queue
         {
            auto capture = local::execute( R"(casual queue --list-forward-queues --porcelain true | awk -F'|' '{printf $11}')");
            EXPECT_EQ( capture.standard.out, "D") << CASUAL_NAMED_VALUE( capture);
         }

         // enable the group
         {
            auto capture = administration::unittest::cli::command::execute( "casual configuration --enable-groups disabled-group");
            EXPECT_TRUE( capture.exit == 0);
         }

         // forward service
         {
            auto capture = local::execute( R"(casual queue --list-forward-services --porcelain true | awk -F'|' '{printf $12}')");
            EXPECT_EQ( capture.standard.out, "E") << CASUAL_NAMED_VALUE( capture);
         }

         // forward queue
         {
            auto capture = local::execute( R"(casual queue --list-forward-queues --porcelain true | awk -F'|' '{printf $11}')");
            EXPECT_EQ( capture.standard.out, "E") << CASUAL_NAMED_VALUE( capture);
         }
      }

      TEST( cli_queue, list_fanout_groups)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( R"(
domain:
   name: A
   queue:
      fanout:
         groups:
            -  alias: fanout-1
               queues:
                  -  source: a
                     targets:
                        -  queue: b
                        -  queue: c
                  -  source: d
                     targets:
                        -  queue: e
            -  alias: fanout-2
               queues:
                  -  source: f
                     targets:
                        -  queue: g

)");

/*
alias     pid    queues  commits  rollbacks  last
--------  -----  ------  -------  ---------  ----
fanout-1  11203       2        0          0  -   
fanout-2  11204       1        0          0  -   
*/


         auto capture = local::execute( R"(casual --color false --header false queue fanout --list-groups)");

         auto rows = string::split( capture.standard.out, '\n');

         EXPECT_TRUE( std::regex_match( rows.at( 0), std::regex{ R"(fanout-1[ ]+\d+[ ]+\d+[ ]+\d+[ ]+\d+[ ]+-[ ]*)"})) << rows.at( 0);
         EXPECT_TRUE( std::regex_match( rows.at( 1), std::regex{ R"(fanout-2[ ]+\d+[ ]+\d+[ ]+\d+[ ]+\d+[ ]+-[ ]*)"})) << rows.at( 1);

      }

      TEST( cli_queue, list_fanout_queues)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( R"(
domain:
   name: A
   groups:
      -  name: g1
         dependencies: [ queue]
      -  name: g2
         dependencies: [ queue]
         enabled: false
   queue:
      groups:
         -  alias: S
            queuebase: ":memory:"
            queues:
               -  name: s1
               -  name: s2
               -  name: s3
               -  name: s4
         -  alias: T
            queuebase: ":memory:"
            queues:
               -  name: t1
               -  name: t2
               -  name: t3
               -  name: t4
               -  name: t5
               -  name: t6
                  
      fanout:
         groups:
            -  alias: fanout-1
               queues:
                  -  alias: foo
                     memberships:
                        -  g1
                     instances: 3
                     source: s1
                     targets:
                        -  queue: t1
                        -  queue: t2
                  -  source: s2
                     instances: 2
                     targets:
                        -  queue: t3
            -  alias: fanout-2
               queues:
                  -  source: s3
                     targets:
                        -  queue: t4
                        -  queue: t5
                        -  queue: t6
                  -  alias: bar
                     instances: 1
                     memberships:
                        -  g2
                     source: s4
                     targets:
                        -  queue: t1
)");  

      
         
         // some tests to see how it looks with some activity
         {
            //EXPECT_TRUE( local::execute( R"(echo "casual" | casual buffer --compose | casual queue --enqueue s1 | casual pipe --human-sink | wc -l)"));
            //EXPECT_TRUE( local::execute( R"(echo "casual" | casual buffer --compose | casual queue --enqueue s2 | casual pipe --human-sink | wc -l)"));
            //EXPECT_TRUE( local::execute( R"(echo "casual" | casual buffer --compose | casual queue --enqueue s3 | casual pipe --human-sink | wc -l)"));
            //auto capture = local::execute( R"(casual queue fanout --list-queues)");
            //EXPECT_TRUE( false) << capture.standard.out;
/*
alias  group     source  T#  S  CI  I  commits  rollbacks  last                            
-----  --------  ------  --  -  --  -  -------  ---------  --------------------------------
bar    fanout-2  s4      1   D   1  0        0          0  -                               
foo    fanout-1  s1      2   E   3  3        1          0  2025-12-27T13:59:55.720895+01:00
s2     fanout-1  s2      1   E   2  2        1          0  2025-12-27T13:59:55.742502+01:00
s3     fanout-2  s3      3   E   1  1        1          0  2025-12-27T13:59:55.764445+01:00                            
*/
         }

         auto capture = local::execute( R"(casual --color false --header false queue fanout --list-queues)");
         auto rows = string::split( capture.standard.out, '\n');

         EXPECT_TRUE( std::regex_match( rows.at( 0), std::regex{ R"(bar    fanout-2  s4      1   D   1  0[ ]+\d+[ ]+\d+[ ]+-[ ]*)"})) << rows.at( 0);
         EXPECT_TRUE( std::regex_match( rows.at( 1), std::regex{ R"(foo    fanout-1  s1      2   E   3  3[ ]+\d+[ ]+\d+[ ]+-[ ]*)"})) << rows.at( 1);
         EXPECT_TRUE( std::regex_match( rows.at( 2), std::regex{ R"(s2     fanout-1  s2      1   E   2  2[ ]+\d+[ ]+\d+[ ]+-[ ]*)"})) << rows.at( 2);
         EXPECT_TRUE( std::regex_match( rows.at( 3), std::regex{ R"(s3     fanout-2  s3      3   E   1  1[ ]+\d+[ ]+\d+[ ]+-[ ]*)"})) << rows.at( 3);
         
      }

      TEST( cli_queue, list_fanout_targets)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( R"(
domain:
   name: A
   queue:
      groups:
         -  alias: S
            queuebase: ":memory:"
            queues:
               -  name: s1
               -  name: s2
               -  name: s3
         -  alias: T
            queuebase: ":memory:"
            queues:
               -  name: t1
               -  name: t2
               -  name: t3
               -  name: t4
               -  name: t5
               -  name: t6
                  
      fanout:
         groups:
            -  alias: fanout-1
               queues:
                  -  alias: foo
                     instances: 3
                     source: s1
                     targets:
                        -  queue: t1
                        -  queue: t2
                  -  source: s2
                     instances: 2
                     targets:
                        -  queue: t3
            -  alias: fanout-2
               queues:
                  -  source: s3
                     targets:
                        -  queue: t4
                        -  queue: t5
                        -  queue: t6
)");  

      
         
         // How the output looks
         {
            //auto capture = local::execute( R"(casual queue --list-fanout-targets)");
            //EXPECT_TRUE( false) << capture.standard.out;
/*
alias  source  target  delay
-----  ------  ------  -----
foo    s1      t1      0.000
foo    s1      t2      0.000
s2     s2      t3      0.000
s3     s3      t4      0.000
s3     s3      t5      0.000
s3     s3      t6      0.000
*/
         }

         auto capture = local::execute( R"(casual --precision 3 --color false --header false queue fanout --list-targets)");

         auto rows = string::split( capture.standard.out, '\n');

         EXPECT_TRUE( std::regex_match( rows.at( 0), std::regex{ R"(foo    s1      t1      0.000)"})) << rows.at( 0);
         EXPECT_TRUE( std::regex_match( rows.at( 1), std::regex{ R"(foo    s1      t2      0.000)"})) << rows.at( 1);
         EXPECT_TRUE( std::regex_match( rows.at( 2), std::regex{ R"(s2     s2      t3      0.000)"})) << rows.at( 2);
         EXPECT_TRUE( std::regex_match( rows.at( 3), std::regex{ R"(s3     s3      t4      0.000)"})) << rows.at( 3);
         EXPECT_TRUE( std::regex_match( rows.at( 4), std::regex{ R"(s3     s3      t5      0.000)"})) << rows.at( 4);
         EXPECT_TRUE( std::regex_match( rows.at( 5), std::regex{ R"(s3     s3      t6      0.000)"})) << rows.at( 5);

      }


   } // administration
} // casual
