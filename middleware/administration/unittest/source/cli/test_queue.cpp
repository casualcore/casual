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
               return administration::unittest::cli::command::execute( std::forward< Cs>( commands)...).consume();
            }
         } // <unnamed>
      } // local

      TEST( cli_queue, attributes)
      {
         common::unittest::Trace trace;

         // reply
         {
            auto output = local::execute( R"(echo "casual" | casual buffer --compose | casual queue --attributes reply b.error | casual pipe --human-sink)");
            EXPECT_TRUE( std::regex_match( output, std::regex{ R"(.*reply: b.error.*\n)"})) << CASUAL_NAMED_VALUE( output); 
         }

         // properties
         {
            auto output = local::execute( R"(echo "casual" | casual buffer --compose | casual queue --attributes properties foo | casual pipe --human-sink)");
            EXPECT_TRUE( std::regex_match( output, std::regex{ R"(.*properties: foo,.*\n)"})) << CASUAL_NAMED_VALUE( output); 
         }

         // available
         {
            auto output = local::execute( R"(echo "casual" | casual buffer --compose | casual queue --attributes available 42s | casual pipe --human-sink)");
            EXPECT_TRUE( std::regex_match( output, std::regex{ R"(.*available: 42.*\n)"})) << CASUAL_NAMED_VALUE( output); 
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
            auto output = local::execute( R"(echo "casual" | casual buffer --compose | casual queue --attributes reply b.error | casual queue --enqueue a | casual pipe --human-sink)");
            EXPECT_TRUE( std::regex_match( output, std::regex{ R"([0-9a-f]{32}\n)"})) << CASUAL_NAMED_VALUE( output);             
         }

         // dequeue
         {
            auto output = local::execute( R"(casual queue --dequeue a | casual pipe --human-sink)");
            EXPECT_TRUE( std::regex_match( output, std::regex{ R"(.*reply: b.error.*\n)"})) << CASUAL_NAMED_VALUE( output); 
         }
      }

   } // administration
} // casual