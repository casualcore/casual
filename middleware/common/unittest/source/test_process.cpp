//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "common/process.h"

#include "common/code/signal.h"
#include "common/code/casual.h"
#include "common/signal.h"


namespace casual
{
   namespace common
   {
      namespace local
      {
         namespace
         {
            auto processPath()
            {
               return std::filesystem::path{ __FILE__}.parent_path().parent_path().parent_path() / "bin" / "simple_process";
            }
         }
      }

      TEST( common_process, handle_equality)
      {
         common::unittest::Trace trace;

         auto handle = process::handle();

         EXPECT_TRUE( handle);
         EXPECT_TRUE( handle == handle.pid);
         EXPECT_TRUE( handle.pid == handle);
         EXPECT_TRUE( handle == handle.ipc);
         EXPECT_TRUE( handle.ipc == handle);
      }

      TEST( common_process, spawn_one_process)
      {
         common::unittest::Trace trace;

         auto pid = process::spawn( local::processPath(), {});

         EXPECT_TRUE( pid);
         EXPECT_TRUE( pid != process::id());

         // wait for it..
         EXPECT_TRUE( process::wait( pid) == 0);
      }

      TEST( common_process, spawn_one_process_with_argument)
      {
         common::unittest::Trace trace;

         auto pid = process::spawn( local::processPath(), { "-r", "42" });

         EXPECT_TRUE( pid);
         EXPECT_TRUE( pid != process::id());

         // wait for it..
         auto result = process::wait( pid);
         EXPECT_TRUE( result == 42) << "result: " << result;

      }

      TEST( common_process, spawn_one_process_check_termination)
      {
         common::unittest::Trace trace;

         log::line( log::category::error, "REMOVE");

         auto pid = process::spawn( local::processPath(), {});

         EXPECT_TRUE( pid);
         EXPECT_TRUE( pid != process::id());

         auto terminated = process::lifetime::ended();

         // We wait for signal that the child died

         ASSERT_CODE(
         {
            process::sleep( std::chrono::seconds{ 2});
         }, common::code::signal::child);

         terminated = process::lifetime::ended();

         ASSERT_TRUE( terminated.size() == 1) << "terminated.size(): " << terminated.size();
         EXPECT_TRUE( terminated.front().pid == pid);
      }


      TEST( common_process, spawn_10_process__children_terminate)
      {
         common::unittest::Trace trace;

         std::vector< strong::process::id> pids( 10);

         for( auto& pid : pids)
         {
            pid = process::spawn( "sleep", { "3600"});
         }

         // TODO posix_spawn seems to have a _setup period_ when signals is "lost" - we mitigate this by sleeping a while...
         std::this_thread::sleep_for( std::chrono::milliseconds{ 100});
        
         auto terminated = process::lifetime::terminate( pids, std::chrono::seconds( 5));

         ASSERT_TRUE( pids.size() == terminated.size());
         EXPECT_TRUE( algorithm::equal( algorithm::sort( pids), algorithm::sort( terminated)));
      }


      TEST( common_process, wait_timeout_non_existing_children)
      {
         common::unittest::Trace trace;

         auto terminated = process::lifetime::wait( { strong::process::id{ 666}});

         EXPECT_TRUE( terminated.empty());
      }

      TEST( common_process, spawn_non_existing_application__gives_exception)
      {
         common::unittest::Trace trace;

         auto path = local::processPath();
         path += "_non_existing_file";

         EXPECT_CODE({
            process::spawn( path, {});
         }, code::casual::invalid_path);
      }

   }
}




