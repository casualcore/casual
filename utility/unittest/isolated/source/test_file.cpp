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

      TEST( casual_utility_file, basename_normal)
      {
         std::string verify = utility::file::basename( "/base/file/name.test");

         EXPECT_TRUE(verify=="name.test") << verify;
      }

      TEST( casual_utility_file, basename_empty_string)
      {
         std::string verify = utility::file::basename( "");

         EXPECT_TRUE(verify=="") << verify;
      }

      TEST( casual_utility_file, basename_only_filename)
      {
         std::string verify = utility::file::basename( "name");

         EXPECT_TRUE(verify=="name") << verify;
      }

      TEST( casual_utility_file, basename_only_filename_with_dot)
      {
         std::string verify = utility::file::basename( "name.");

         EXPECT_TRUE(verify=="name.") << verify;
      }

      TEST( casual_utility_file, extension_normal)
      {
         std::string verify = utility::file::extension( "/path/to/filename/file.testextension");

         EXPECT_TRUE(verify=="testextension") << verify;
      }

      TEST(casual_utility_file,extension_no_extension)
      {
         std::string verify = utility::file::extension( "/path/to/filename/file");
         EXPECT_TRUE( verify == "") << verify;
      }

      TEST(casual_utility_file,extension_no_extension_dot_in_path)
      {
         std::string verify = utility::file::extension( "/path/to/.filename/file");
         EXPECT_TRUE( verify == "") << verify;
      }

      TEST(casual_utility_file,extension_empty_path)
      {
         std::string verify = utility::file::extension( "");
         EXPECT_TRUE( verify == "") << verify;
      }

      TEST(casual_utility_file, basedir_normal)
      {
         std::string verify = utility::file::basedir( "/base/dir/file.name");
         EXPECT_TRUE(verify == "/base/dir/") << verify;
      }

      TEST(casual_utility_file, basedir_empty_path)
      {
         std::string verify = utility::file::basedir( "");
         EXPECT_TRUE(verify == "") << verify;
      }

      TEST(casual_utility_file, basedir_only_filename_path)
      {
         std::string verify = utility::file::basedir( "file.name");
         EXPECT_TRUE(verify == "") << verify;
      }

      TEST(casual_utility_file,find_existing_file)
      {
         std::string path = utility::file::basedir( __FILE__);
         std::string file = utility::file::basename( __FILE__);

         std::regex regex = std::regex( file);

         std::string verify = utility::file::find( path, regex);
         EXPECT_TRUE( verify == file) << verify;
      }

      TEST(casual_utility_file,find_nonexisting_file)
      {
         std::string path = utility::file::basedir( __FILE__);
         std::string file = utility::file::basename( __FILE__) + "testfail";

         std::regex regex = std::regex( file);

         std::string verify = utility::file::find( path, regex);
         EXPECT_TRUE( verify == "") << verify;
      }

      TEST(casual_utility_file,find_empty_arguments)
      {
         std::string path = utility::file::basedir( "");
         std::string file = utility::file::basename( "");

         std::regex regex = std::regex( file);

         std::string verify = utility::file::find( path, regex);
         EXPECT_TRUE( verify == "") << verify;
      }
   }
}
