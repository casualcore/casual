//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"
#include "common/unittest/file.h"

#include "common/file.h"
#include "common/environment.h"
#include "common/uuid.h"
#include "common/algorithm/is.h"


#include <fstream>

namespace casual
{

   namespace common
   {

      TEST( casual_common_file, basename_normal)
      {
         common::unittest::Trace trace;

         std::string verify = common::file::name::base( "/base/file/name.test");

         EXPECT_TRUE(verify=="name.test") << verify;
      }

      TEST( casual_common_file, basename_empty_string)
      {
         common::unittest::Trace trace;

         std::string verify = common::file::name::base( "");

         EXPECT_TRUE(verify=="") << verify;
      }

      TEST( casual_common_file, basename_only_filename)
      {
         common::unittest::Trace trace;

         std::string verify = common::file::name::base( "name");

         EXPECT_TRUE(verify=="name") << verify;
      }

      TEST( casual_common_file, basename_only_filename_with_dot)
      {
         common::unittest::Trace trace;

         std::string verify = common::file::name::base( "name.");

         EXPECT_TRUE(verify=="name.") << verify;
      }

      TEST( casual_common_file, extension_normal)
      {
         common::unittest::Trace trace;

         std::string verify = common::file::name::extension( "/path/to/filename/file.testextension");

         EXPECT_TRUE(verify=="testextension") << verify;
      }

      TEST( common_file,extension_no_extension)
      {
         common::unittest::Trace trace;

         std::string verify = common::file::name::extension( "/path/to/filename/file");
         EXPECT_TRUE( verify == "") << verify;
      }

      TEST( common_file,extension_no_extension_dot_in_path)
      {
         common::unittest::Trace trace;

         auto verify = common::file::name::extension( "/path/to/.filename/file");
         EXPECT_TRUE( verify == "") << verify;
      }

      TEST( common_file,extension_empty_path)
      {
         common::unittest::Trace trace;

         auto verify = common::file::name::extension( "");
         EXPECT_TRUE( verify == "") << verify;
      }

      TEST( common_file, basedir_normal)
      {
         common::unittest::Trace trace;

         auto verify = common::directory::name::base( "/base/dir/file.name");
         EXPECT_TRUE(verify == "/base/dir/") << verify;
      }

      TEST( common_file, basedir_empty_path)
      {
         common::unittest::Trace trace;

         auto verify = common::directory::name::base( "");
         EXPECT_TRUE(verify == "/") << verify;
      }

      TEST( common_file, basedir_only_filename_path)
      {
         common::unittest::Trace trace;

         std::string verify = common::directory::name::base( "file.name");
         // TODO: this should be '.'
         EXPECT_TRUE(verify == "/") << verify;
      }

      TEST( common_file_find_regex, existing_file)
      {
         common::unittest::Trace trace;

         auto path = common::directory::name::base( __FILE__);
         auto file = common::file::name::base( __FILE__);

         std::regex regex = std::regex( file);

         std::string verify = common::file::find( path, regex);
         EXPECT_TRUE( verify == file) << verify;
      }


      TEST( common_file_find_regex, nonexisting_file)
      {
         common::unittest::Trace trace;

         std::string path = common::directory::name::base( __FILE__);
         std::string file = common::file::name::base( __FILE__) + "testfail";

         std::regex regex = std::regex( file);

         std::string verify = common::file::find( path, regex);
         EXPECT_TRUE( verify == "") << verify;
      }

      TEST( common_file_find_regex, empty_arguments)
      {
         common::unittest::Trace trace;

         std::string path = common::directory::name::base( "");
         std::string file = common::file::name::base( "");

         std::regex regex = std::regex( file);

         std::string verify = common::file::find( path, regex);
         EXPECT_TRUE( verify == "") << verify;
      }

      TEST( common_file_find_pattern, existing_files)
      {
         common::unittest::Trace trace;
         
         auto paths = common::file::find( common::directory::name::base( __FILE__) + "/test_*.cpp");

         EXPECT_TRUE( paths.size() > 1) << "paths :" << paths;
      }

      TEST( common_file_find_pattern, specific_file)
      {
         common::unittest::Trace trace;
         
         auto paths = common::file::find( __FILE__);
         EXPECT_TRUE( paths.size() == 1) << "paths :" << paths;
         EXPECT_TRUE( paths.at( 0) == __FILE__) << "paths :" << paths;
      }

      TEST( common_file_find_pattern, nonexistent_path)
      {
         common::unittest::Trace trace;

         auto pattern = common::file::name::unique( "/a/b/c/", "/*");
         
         auto paths = common::file::find( pattern);
         EXPECT_TRUE( paths.empty()) << "paths :" << paths;
      }

      TEST( common_file_find_pattern, empty_pattern__expect_no_found)
      {
         common::unittest::Trace trace;
         
         auto paths = common::file::find( "");

         EXPECT_TRUE( paths.empty()) << "paths :" << paths;
      }

      TEST( common_file_find_pattern, same_pattern_twice__expect_unique_paths)
      {
         common::unittest::Trace trace;

         const auto pattern = common::directory::name::base( __FILE__) + "test_*.cpp";
         
         auto paths = common::file::find( std::vector< std::string>{ pattern, pattern});

         EXPECT_TRUE( paths.size() > 1) << "paths :" << paths;
         EXPECT_TRUE( algorithm::is::unique( paths)) << "paths :" << paths;
         EXPECT_TRUE( common::file::find( pattern) == paths) << "paths :" << paths;
      }


      TEST( common_file_find_pattern, two_patterns__expect_preserved_order_between_the_patterns)
      {
         common::unittest::Trace trace;

         unittest::directory::temporary::Scoped dir;
         file::Output a{ dir.path() + "/a_foo.yaml"};
         file::Output b{ dir.path() + "/b_foo.yaml"};
         
         auto paths = common::file::find( std::vector< std::string>{ dir.path() + "/b*.yaml", dir.path() + "/a*.yaml"});
         ASSERT_TRUE( paths.size() == 2) << "paths :" << paths;
         EXPECT_TRUE( paths.at( 0) == b.path()) << "paths :" << paths;
         EXPECT_TRUE( paths.at( 1) == a.path()) << "paths :" << paths;
      }

      TEST( common_file_find_pattern, two_files__expect_sorted_order)
      {
         common::unittest::Trace trace;

         unittest::directory::temporary::Scoped dir;
         file::Output b{ dir.path() + "/b_foo.yaml"};
         file::Output a{ dir.path() + "/a_foo.yaml"};
         
         auto paths = common::file::find( dir.path() + "/*.yaml");
         ASSERT_TRUE( paths.size() == 2) << "paths :" << paths;
         EXPECT_TRUE( paths.at( 0) == a.path()) << "paths :" << paths;
         EXPECT_TRUE( paths.at( 1) == b.path()) << "paths :" << paths;
      }


      TEST( common_file, exists__expect_true)
      {
         common::unittest::Trace trace;

         EXPECT_TRUE( common::file::exists( __FILE__));
      }

      TEST( common_file, exists__expect_false)
      {
         common::unittest::Trace trace;

         EXPECT_FALSE( common::file::exists( std::string( __FILE__) + "_not_a_file_"));
      }

      TEST( common_file, move_file__expect_success)
      {
         common::unittest::Trace trace;

         auto path = file::scoped::Path{ file::name::unique( "/tmp/", ".txt")};
         {
            std::ofstream out{ path};
            out << "poop\n";
         }

         EXPECT_TRUE( common::file::exists( path));

         auto destination = file::scoped::Path{ file::name::unique( "/tmp/", ".txt")};

         EXPECT_NO_THROW({
            file::move( path, destination);
         });
      }

      TEST( common_file, move_file__expect_throw)
      {
         common::unittest::Trace trace;

         auto path = file::scoped::Path{ file::name::unique( "/tmp/", ".txt")};
         {
            std::ofstream out{ path};
            out << "poop\n";
         }

         EXPECT_TRUE( common::file::exists( path));

         auto destination = file::scoped::Path{ file::name::unique( "/", "/non-existent-path.txt")};

         EXPECT_CODE(
         {
            file::move( path, destination);
         }, code::casual::invalid_path);
      }

      TEST( common_directory, create_one_level__expect_true)
      {
         common::unittest::Trace trace;

         std::string path( environment::directory::temporary() + "/test_create_recursive_" + uuid::string( uuid::make()));

         EXPECT_TRUE( directory::create( path));
         EXPECT_TRUE( directory::remove( path));
      }

      TEST( common_directory, create_3_level__expect_true)
      {
         common::unittest::Trace trace;

         std::string path( environment::directory::temporary() + "/test_create_recursive_" +uuid::string( uuid::make()) + "/level2/level3");

         EXPECT_TRUE( directory::create( path));

         EXPECT_TRUE( directory::remove( path));

         path = directory::name::base( path);
         EXPECT_TRUE( directory::remove( path));

         path = directory::name::base( path);
         EXPECT_TRUE( directory::remove( path));
      }




   } // common
} // casual
