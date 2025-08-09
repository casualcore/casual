//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#define CASUAL_NO_XATMI_UNDEFINE

#include "common/unittest.h"
#include "common/unittest/file.h"

#include "transaction/context.h"

#include "administration/unittest/cli/command.h"

#include "domain/unittest/manager.h"

#include "gateway/unittest/utility.h"

#include "queue/unittest/utility.h"
#include "queue/api/queue.h"

#include "common/string.h"
#include "common/sink.h"

#include "file/api/file.h"

#include <regex>

namespace casual
{
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
      -  name: file
         dependencies: [ base]
   
   servers:
      - path: "${CMAKE_BINARY_DIR}/middleware/service/bin/casual-service-manager"
        memberships: [ base]
      - path: "${CMAKE_BINARY_DIR}/middleware/transaction/bin/casual-transaction-manager"
        memberships: [ base]
      - path: "${CMAKE_BINARY_DIR}/middleware/file/bin/casual-file-manager"
        memberships: [ file]
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
         } //
      } // local

      TEST( cli_file, list_reservations)
      {
/*
casual file --list-reservations

path                                                  pid     gtrid                             stage    time
----------------------------------------------------  ------  --------------------------------  -------  --------------------------
"/tmp/unittest-bf74fa17faec4b5db228d46eb4724162.txt"  244765  34cf0cef48dc47748d78a5ec40484dfc  working  2025-06-05 07:26:07.366325
*/

         common::unittest::Trace trace;

         auto domain = local::domain();

         const auto file = common::unittest::file::temporary::name( ".txt");

         ASSERT_EQ( casual::transaction::context().begin(), common::code::tx::ok);

         const auto reserved = file::blocking::reserve( file);

         const auto capture = local::execute( "casual file --list-reservations --porcelain true").standard.out;

         // Check that expected state and path exists
         EXPECT_TRUE( capture.contains( "working")) << capture;
         EXPECT_TRUE( capture.contains( file.native())) << capture;

         ASSERT_EQ( casual::transaction::context().commit(), common::code::tx::ok);
      }

      TEST( cli_file, recover_transaction)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         const auto file = common::unittest::file::temporary::name( ".txt");

         ASSERT_EQ( casual::transaction::context().begin(), common::code::tx::ok);

         const auto reserved = file::blocking::reserve( file);

         // Check that there's some reservations
         {
            const auto capture = local::execute( "casual file --list-reservations --porcelain true").standard.out;

            EXPECT_FALSE( capture.empty()) << capture;
         }

         auto& transaction = casual::transaction::context().current();
         
         ASSERT_TRUE( transaction);

         // Commit the reservaton and check that something is returned
         {
            const auto gtrid = common::string::compose( transaction.trid.global());
            const auto capture = local::execute( "casual file --recover-transactions --commit " + gtrid).standard.out;
            EXPECT_FALSE( capture.empty()) << capture;
         }

         // Check that there are no reservations
         {
            const auto capture = local::execute( "casual file --list-reservations --porcelain true").standard.out;
            EXPECT_TRUE( capture.empty()) << capture;
         }

         ASSERT_EQ( casual::transaction::context().commit(), common::code::tx::ok);
      }

      TEST( cli_file, produce_consume)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( R"(
domain:
   name: A
   servers:
      - path: "${CMAKE_BINARY_DIR}/middleware/queue/bin/casual-queue-manager"
        memberships: [ base]
   queue:
      groups:
         -  alias: Q
            queues:
               - name: a
               - name: b
            
)");
         // add som messages to queue 'a'
         {
            auto capture = local::execute( R"(echo -n "casual-message-1" | casual buffer --compose '.string/' | casual buffer --duplicate 5 | casual queue --enqueue a | casual pipe --human-sink)");
            EXPECT_TRUE( capture) << CASUAL_NAMED_VALUE( capture); // just to have the output if something fails

            auto messages = queue::unittest::messages( "a");
            EXPECT_TRUE( messages.size() == 5) << CASUAL_NAMED_VALUE( messages);
         }

         common::unittest::directory::temporary::Scoped directory;

         // produce files from queue 'a'
         {
            auto capture = local::execute( "casual transaction --begin | casual queue --consume a | casual file --produce " + directory.path().string() + " | casual transaction --commit ");
            EXPECT_TRUE( capture) << CASUAL_NAMED_VALUE( capture); // just to have the output if something fails
         }

         // check that we have files in the directory
         {
            auto files = common::unittest::directory::list( directory.path());
            EXPECT_TRUE( files.size() == 5) << CASUAL_NAMED_VALUE( files);
         }
         
         // consume files into queue 'b'
         {
            auto capture = local::execute( "casual transaction --begin | casual file --consume " + (directory.path() / "*").string() + " | casual queue --enqueue b | casual transaction --commit | casual pipe --human-sink ");
            EXPECT_TRUE( capture) << CASUAL_NAMED_VALUE( capture); // just to have the output if something fails
         }

         // expect the directory to be empty
         {
            auto files = common::unittest::directory::list( directory.path());
            EXPECT_TRUE( files.empty()) << CASUAL_NAMED_VALUE( files);
         }

         // check that the files has been enqueued into 'b'
         {
            auto messages = queue::unittest::messages( "b");
            EXPECT_TRUE( messages.size() == 5) << CASUAL_NAMED_VALUE( messages);

            // browse messages in 'b' to check content
            queue::browse::peek( "b", []( queue::Message&& message)
            {
               auto string_content = std::string_view{ common::binary::span::to_string_like( message.payload.data)};
               EXPECT_TRUE( string_content == "casual-message-1") << "content: '" << string_content << "'";

               return true;
            });
         }

      }

   } // administration
} // casual
