//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

//! This test-suite is intended for compound command line pipe "expressions". 
//! We test the whole _casual-pipe_ semantic, not a particular module.

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
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager"
         memberships: [ base]
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/casual-transaction-manager"
         memberships: [ base]
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/queue/bin/casual-queue-manager"
         memberships: [ queue]
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]
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
               return administration::unittest::cli::command::execute( std::forward< Cs>( commands)...).consume();
            }
         } // <unnamed>
      } // local

      TEST( cli_pipe, log_file_to_stdout__expect_cli_error)
      {
         auto domain = local::domain();

          auto output = local::execute( R"(CASUAL_LOG_PATH=/dev/stdout casual service --list-services 2>&1 )");

          EXPECT_TRUE( algorithm::search( output, std::string_view( "casual:precondition"))) << output;

      }

      TEST( cli_pipe, enqueue_dequeue_call_echo)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( R"(
domain:
   name: A
   queue:
      groups:
         -  alias: QA
            queuebase: ":memory:"
            queues:
               -  name: a1
)");

         auto output = local::execute( R"(echo "casual" | casual buffer --compose | casual queue --enqueue a1 | casual queue --dequeue a1 | casual call --service casual/example/echo | casual buffer --extract)");
         EXPECT_TRUE( output == "casual\n") << CASUAL_NAMED_VALUE( output);
      }

      TEST( cli_pipe, trans_begin__enqueue__trans_commit__dequeue__expect_message)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( R"(
domain:
   name: A
   queue:
      groups:
         -  alias: QA
            queuebase: ":memory:"
            queues:
               -  name: a1

)");

         auto output = local::execute( R"(echo "casual" | casual buffer --compose | casual transaction --begin | casual queue --enqueue a1 | casual transaction --commit | casual queue --dequeue a1 | casual buffer --extract)");
         EXPECT_TRUE( output == "casual\n") << CASUAL_NAMED_VALUE( output);
      }

      TEST( cli_pipe, trans_begin__enqueue___dequeue__expect_NO_message)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( R"(
domain:
   name: A
   queue:
      groups:
         -  alias: QA
            queuebase: ":memory:"
            queues:
               -  name: a1
)");

         // note that we do commit after the dequeue, otherwise the queue-group will resist to shutdown. 
         auto output = local::execute( R"(echo "casual" | casual buffer --compose | casual transaction --begin | casual queue --enqueue a1 | casual queue --dequeue a1 | casual transaction --commit | casual buffer --extract)");
         EXPECT_TRUE( output.empty()) << CASUAL_NAMED_VALUE( output);
      }

      TEST( cli_pipe, trans_begin__enqueue__trans_rollback___dequeue__expect_NO_message)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( R"(
domain:
   name: A
   queue:
      groups:
         -  alias: QA
            queuebase: ":memory:"
            queues:
               -  name: a1
)");

         // note that we do commit after the dequeue, otherwise the queue-group will resist to shutdown. 
         auto output = local::execute( R"(echo "casual" | casual buffer --compose | casual transaction --begin | casual queue --enqueue a1 |  casual transaction --rollback | casual queue --dequeue a1 | casual buffer --extract)");
         EXPECT_TRUE( output.empty()) << CASUAL_NAMED_VALUE( output);
      }


      TEST( cli_pipe, enqueue_a1_trans_begin__dequeue_a1__call_TPESVCFAIL__enqueue_a2__trans_commit__expect_rollback_to_a1_dequeue_a1)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( R"(
domain:
   name: A
   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-error-server"
         memberships: [ user]
   queue:
      groups:
         -  alias: QA
            queuebase: ":memory:"
            queues:
               -  name: a1
                  retry:
                     count: 10
               -  name: a2
)");


         // note that we do commit after the dequeue, otherwise the queue-group will resist to shutdown. 
         auto output = local::execute( R"(echo "casual" | casual buffer --compose | casual queue --enqueue a1 | casual transaction --begin | casual queue --dequeue a1 | casual call --service casual/example/error/TPESVCFAIL | casual queue --enqueue a2 | casual transaction --rollback | casual queue --dequeue a1 | casual buffer --extract)");
         EXPECT_TRUE( output == "casual\n") << CASUAL_NAMED_VALUE( output);

      }


      TEST( cli_pipe, stacked_transaction_commit)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( R"(
domain:
   name: A
   queue:
      groups:
         -  alias: QA
            queuebase: ":memory:"
            queues:
               -  name: a1
               -  name: a2
               -  name: a3
)");


         constexpr std::string_view command = R"(echo "stacked transactions" | casual buffer --compose \
| casual transaction --begin \
| casual transaction --begin | casual queue --enqueue a1 | casual transaction --commit \
| casual transaction --begin | casual queue --dequeue a1 | casual queue --enqueue a2 | casual transaction --commit \
| casual queue --dequeue a2 | casual queue --enqueue a3 \
| casual transaction --commit \
| casual queue --dequeue a3 | casual buffer --extract \
)";

         auto output = local::execute( command);
         EXPECT_TRUE( output == "stacked transactions\n") << CASUAL_NAMED_VALUE( output);
      }

   
   } // administration
} // casual