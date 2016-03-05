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

#include "common/trace.h"

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
               return directory::name::base( __FILE__) + "../../../bin/simple_process";
            }

         }
      }

      TEST( casual_common_process, spawn_one_process)
      {
         Trace trace{ "TEST( casual_common_process, spawn_one_process)", log::internal::trace};

         auto pid = process::spawn( local::processPath(), {});

         EXPECT_TRUE( pid != 0);
         EXPECT_TRUE( pid != process::id());

         // wait for it..
         EXPECT_TRUE( process::wait( pid) == 0);

         signal::clear();
      }

      TEST( casual_common_process, spawn_one_process_with_argument)
      {
         Trace trace{ "TEST( casual_common_process, spawn_one_process_with_argument)", log::internal::trace};

         auto pid = process::spawn( local::processPath(), { "-r", "42" });

         EXPECT_TRUE( pid != 0);
         EXPECT_TRUE( pid != process::id());

         // wait for it..
         auto result = process::wait( pid);
         EXPECT_TRUE( result == 42) << "result: " << result;

         signal::clear();
      }

      TEST( casual_common_process, spawn_one_process_check_termination)
      {
         Trace trace{ "TEST( casual_common_process, spawn_one_process_check_termination)", log::internal::trace};

         auto pid = process::spawn( local::processPath(), {});

         EXPECT_TRUE( pid != 0);
         EXPECT_TRUE( pid != process::id());

         auto terminated = process::lifetime::ended();

         //
         // We wait for signal that the child died
         //
         try
         {
            process::sleep( std::chrono::seconds( 2));
         }
         catch( const exception::signal::child::Terminate&)
         {
            terminated = process::lifetime::ended();
         }

         ASSERT_TRUE( terminated.size() == 1) << "terminated.size(): " << terminated.size();
         EXPECT_TRUE( terminated.front().pid == pid);

         signal::clear();
      }


      TEST( casual_common_process, spawn_10_process__children_terminate)
      {
         Trace trace{ "TEST( casual_common_process, spawn_10_process__children_terminate)", log::internal::trace};

         std::vector< platform::pid_type> pids( 10);

         for( auto& pid : pids)
         {
            pid = process::spawn( local::processPath(), {});
         }

         auto terminated = process::lifetime::terminate( pids, std::chrono::seconds( 5));


         ASSERT_TRUE( pids.size() == terminated.size());
         EXPECT_TRUE( range::equal( range::sort( pids), range::sort( terminated)));

         signal::clear();
      }

      /*
       * doesnt work...
       */
      TEST( casual_common_process, wait_timeout_non_existing_children)
      {
         Trace trace{ "TEST( casual_common_process, wait_timeout_non_existing_children)", log::internal::trace};

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




