//!
//! test_arguments.cpp
//!
//! Created on: Jan 5, 2013
//!     Author: Jon
//!

#include <gtest/gtest.h>

#include "common/file.h"
#include "common/environment.h"
#include "common/uuid.h"

namespace casual
{

   namespace common
   {

      TEST( casual_common_file, basename_normal)
      {
         std::string verify = common::file::basename( "/base/file/name.test");

         EXPECT_TRUE(verify=="name.test") << verify;
      }

      TEST( casual_common_file, basename_empty_string)
      {
         std::string verify = common::file::basename( "");

         EXPECT_TRUE(verify=="") << verify;
      }

      TEST( casual_common_file, basename_only_filename)
      {
         std::string verify = common::file::basename( "name");

         EXPECT_TRUE(verify=="name") << verify;
      }

      TEST( casual_common_file, basename_only_filename_with_dot)
      {
         std::string verify = common::file::basename( "name.");

         EXPECT_TRUE(verify=="name.") << verify;
      }

      TEST( casual_common_file, extension_normal)
      {
         std::string verify = common::file::extension( "/path/to/filename/file.testextension");

         EXPECT_TRUE(verify=="testextension") << verify;
      }

      TEST(casual_common_file,extension_no_extension)
      {
         std::string verify = common::file::extension( "/path/to/filename/file");
         EXPECT_TRUE( verify == "") << verify;
      }

      TEST(casual_common_file,extension_no_extension_dot_in_path)
      {
         std::string verify = common::file::extension( "/path/to/.filename/file");
         EXPECT_TRUE( verify == "") << verify;
      }

      TEST(casual_common_file,extension_empty_path)
      {
         std::string verify = common::file::extension( "");
         EXPECT_TRUE( verify == "") << verify;
      }

      TEST(casual_common_file, basedir_normal)
      {
         std::string verify = common::file::basedir( "/base/dir/file.name");
         EXPECT_TRUE(verify == "/base/dir/") << verify;
      }

      TEST(casual_common_file, basedir_empty_path)
      {
         auto verify = common::file::basedir( "");
         EXPECT_TRUE(verify == "/") << verify;
      }

      TEST(casual_common_file, basedir_only_filename_path)
      {
         std::string verify = common::file::basedir( "file.name");
         // TODO: this should be '.'
         EXPECT_TRUE(verify == "/") << verify;
      }

      TEST(casual_common_file,find_existing_file)
      {
         std::string path = common::file::basedir( __FILE__);
         std::string file = common::file::basename( __FILE__);

         std::regex regex = std::regex( file);

         std::string verify = common::file::find( path, regex);
         EXPECT_TRUE( verify == file) << verify;
      }

      TEST(casual_common_file,find_nonexisting_file)
      {
         std::string path = common::file::basedir( __FILE__);
         std::string file = common::file::basename( __FILE__) + "testfail";

         std::regex regex = std::regex( file);

         std::string verify = common::file::find( path, regex);
         EXPECT_TRUE( verify == "") << verify;
      }

      TEST(casual_common_file,find_empty_arguments)
      {
         std::string path = common::file::basedir( "");
         std::string file = common::file::basename( "");

         std::regex regex = std::regex( file);

         std::string verify = common::file::find( path, regex);
         EXPECT_TRUE( verify == "") << verify;
      }

      TEST(casual_common_file, exists__expect_true)
      {
         EXPECT_TRUE( common::file::exists( __FILE__));
      }

      TEST(casual_common_file, exists__expect_false)
      {
         EXPECT_FALSE( common::file::exists( std::string( __FILE__) + "_not_a_file_"));
      }

      TEST(casual_common_directory, create_one_level__expect_true)
      {
         file::ScopedPath path( environment::directory::temporary() + "/" + Uuid::make().string());

         EXPECT_TRUE( directory::create( path));
      }

      TEST(casual_common_directory, create_two_level__expect_true)
      {
         file::ScopedPath path( environment::directory::temporary() + "/" + Uuid::make().string() + "/level2/level3");

         EXPECT_TRUE( directory::create( path));
      }




   } // common
} // casual
