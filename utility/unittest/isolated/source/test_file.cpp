//!
//! test_arguments.cpp
//!
//! Created on: Jan 5, 2013
//!     Author: Jon
//!

#include <gtest/gtest.h>

#include "utility/file.h"

namespace casual
{

   namespace utility
   {

      TEST( casual_utility_file, basenameNormalBehaviour)
      {
         std::string verify = utility::file::basename( "/base/file/name.test");

         EXPECT_TRUE(verify=="name") << verify;
      }

      TEST( casual_utility_file, basenameEmptyStringBehaviour)
      {
         std::string verify = utility::file::basename( "");

         EXPECT_TRUE(verify=="") << verify;
      }

      TEST( casual_utility_file, basenameNoPathBehaviour)
      {
         std::string verify = utility::file::basename("name");

         EXPECT_TRUE(verify=="name") << verify;
      }

      TEST( casual_utility_file, basenameNoPathWithDotBehaviour)
            {
               std::string verify = utility::file::basename("name.");

               EXPECT_TRUE(verify=="name") << verify;
            }
   }
}
