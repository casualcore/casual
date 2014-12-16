//!
//! test_process.cpp
//!
//! Created on: May 12, 2013
//!     Author: Lazan
//!


#include <gtest/gtest.h>

#include "common/process.h"
#include "common/file.h"
#include "common/exception.h"

#include "common/signal.h"

namespace casual
{
   namespace common
   {
      namespace local
      {
         namespace
         {
            std::string processPath()
            {
               return file::basedir( __FILE__) + "../../../bin/simple_process";
            }

         }
      }

      TEST( casual_common_process, spawn_one_process)
      {
         platform::pid_type pid = process::spawn( local::processPath(), {});

         EXPECT_TRUE( pid != 0);
         EXPECT_TRUE( pid != process::id());

         // wait for it..
         EXPECT_TRUE( process::wait( pid) == 0);

         signal::clear();
      }

      TEST( casual_common_process, spawn_one_process_with_argument)
      {

         platform::pid_type pid = process::spawn( local::processPath(), { "-r", "42" });

         EXPECT_TRUE( pid != 0);
         EXPECT_TRUE( pid != process::id());

         // wait for it..
         pid = process::wait( pid);
         EXPECT_TRUE( pid == 42) << "pid: " << pid;

         signal::clear();
      }

      TEST( casual_common_process, spawn_one_process_check_termination)
      {
         platform::pid_type pid = process::spawn( local::processPath(), {});

         EXPECT_TRUE( pid != 0);
         EXPECT_TRUE( pid != process::id());

         auto terminated = process::lifetime::ended();


         while( terminated.empty())
         {
            process::sleep( std::chrono::milliseconds( 1));
            terminated = process::lifetime::ended();
         }

         ASSERT_TRUE( terminated.size() == 1) << "terminated.size(): " << terminated.size();
         EXPECT_TRUE( terminated.front().pid == pid);

         signal::clear();
      }


      TEST( casual_common_process, spawn_10_process__children_terminate)
      {
         std::vector< platform::pid_type> pids( 10);

         for( auto& pid : pids)
         {
            pid = process::spawn( local::processPath(), {});
         }

         auto terminated = process::lifetime::terminate( pids, std::chrono::seconds( 5));


         ASSERT_TRUE( pids.size() == terminated.size());
         EXPECT_TRUE( range::equal( range::sort( pids), range::sort( range::sort( pids))));

         signal::clear();
      }

      /*
       * doesnt work...
       */
      TEST( casual_common_process, wait_timeout_non_existing_children)
      {
         std::vector< platform::pid_type> pids( 10);

         auto terminated = process::lifetime::wait( { 666});

         EXPECT_TRUE( terminated.empty());

         signal::clear();
      }




      /*
       * does not work right now...
      TEST( casual_common_process, spawn_non_existing_application__gives_exception)
      {
         auto pid = process::spawn( local::processPath() + "_non_existing_file", {});

         EXPECT_THROW({
            process::wait( pid);
         }, exception::Base);

      }
      */
   }
}




