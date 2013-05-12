//!
//! test_process.cpp
//!
//! Created on: May 12, 2013
//!     Author: Lazan
//!


#include <gtest/gtest.h>

#include "common/process.h"
#include "common/file.h"

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
               return file::basedir( __FILE__) + "../../../bin/simple_process_bajs";
            }

         }
      }

      TEST( casual_common_process, spawn_one_process)
      {
         platform::pid_type pid = process::spawn( local::processPath(), std::vector< std::string>{});

         EXPECT_TRUE( pid != 0);
         EXPECT_TRUE( pid != platform::getProcessId());

         // wait for it..
         EXPECT_TRUE( process::wait( pid) == pid);
      }

      TEST( casual_common_process, spawn_one_process_check_termination)
      {
         platform::pid_type pid = process::spawn( local::processPath(), std::vector< std::string>{});

         EXPECT_TRUE( pid != 0);
         EXPECT_TRUE( pid != platform::getProcessId());

         std::vector< platform::pid_type> terminated;

         while( ( terminated = process::terminated()).empty())
         {
            process::sleep( std::chrono::milliseconds( 1));
         }

         ASSERT_TRUE( terminated.size() == 1) << "terminated.size(): " << terminated.size();
         EXPECT_TRUE( terminated.front() == pid);


      }
   }
}




