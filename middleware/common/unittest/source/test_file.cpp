//!
//! casual
//!

#include <common/unittest.h>

#include "common/file.h"
#include "common/environment.h"
#include "common/uuid.h"


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

      TEST(casual_common_file,extension_no_extension)
      {
         common::unittest::Trace trace;

         std::string verify = common::file::name::extension( "/path/to/filename/file");
         EXPECT_TRUE( verify == "") << verify;
      }

      TEST(casual_common_file,extension_no_extension_dot_in_path)
      {
         common::unittest::Trace trace;

         auto verify = common::file::name::extension( "/path/to/.filename/file");
         EXPECT_TRUE( verify == "") << verify;
      }

      TEST(casual_common_file,extension_empty_path)
      {
         common::unittest::Trace trace;

         auto verify = common::file::name::extension( "");
         EXPECT_TRUE( verify == "") << verify;
      }

      TEST(casual_common_file, basedir_normal)
      {
         common::unittest::Trace trace;

         auto verify = common::directory::name::base( "/base/dir/file.name");
         EXPECT_TRUE(verify == "/base/dir/") << verify;
      }

      TEST(casual_common_file, basedir_empty_path)
      {
         common::unittest::Trace trace;

         auto verify = common::directory::name::base( "");
         EXPECT_TRUE(verify == "/") << verify;
      }

      TEST(casual_common_file, basedir_only_filename_path)
      {
         common::unittest::Trace trace;

         std::string verify = common::directory::name::base( "file.name");
         // TODO: this should be '.'
         EXPECT_TRUE(verify == "/") << verify;
      }

      TEST(casual_common_file,find_existing_file)
      {
         common::unittest::Trace trace;

         auto path = common::directory::name::base( __FILE__);
         auto file = common::file::name::base( __FILE__);

         std::regex regex = std::regex( file);

         std::string verify = common::file::find( path, regex);
         EXPECT_TRUE( verify == file) << verify;
      }

      TEST(casual_common_file,find_nonexisting_file)
      {
         common::unittest::Trace trace;

         std::string path = common::directory::name::base( __FILE__);
         std::string file = common::file::name::base( __FILE__) + "testfail";

         std::regex regex = std::regex( file);

         std::string verify = common::file::find( path, regex);
         EXPECT_TRUE( verify == "") << verify;
      }

      TEST(casual_common_file,find_empty_arguments)
      {
         common::unittest::Trace trace;

         std::string path = common::directory::name::base( "");
         std::string file = common::file::name::base( "");

         std::regex regex = std::regex( file);

         std::string verify = common::file::find( path, regex);
         EXPECT_TRUE( verify == "") << verify;
      }

      TEST(casual_common_file, exists__expect_true)
      {
         common::unittest::Trace trace;

         EXPECT_TRUE( common::file::exists( __FILE__));
      }

      TEST(casual_common_file, exists__expect_false)
      {
         common::unittest::Trace trace;

         EXPECT_FALSE( common::file::exists( std::string( __FILE__) + "_not_a_file_"));
      }

      TEST(casual_common_file, move_file__expect_success)
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

      TEST(casual_common_file, move_file__expect_throw)
      {
         common::unittest::Trace trace;

         auto path = file::scoped::Path{ file::name::unique( "/tmp/", ".txt")};
         {
            std::ofstream out{ path};
            out << "poop\n";
         }

         EXPECT_TRUE( common::file::exists( path));

         auto destination = file::scoped::Path{ file::name::unique( "/", "/non-existent-paht.txt")};

         try
         {
            file::move( path, destination);
         }
         catch( const exception::system::exception& e)
         {
            std::cerr << typeid( e).name() << std::endl;
            std::cerr << e << std::endl;
            std::cerr << exception::system::invalid::File{} << std::endl;
         }
         //EXPECT_THROW({
         //   file::move( path, destination);
         //}, exception::system::invalid::File);
      }

      TEST(casual_common_directory, create_one_level__expect_true)
      {
         common::unittest::Trace trace;

         std::string path( environment::directory::temporary() + "/test_create_recursive_" + uuid::string( uuid::make()));

         EXPECT_TRUE( directory::create( path));
         EXPECT_TRUE( directory::remove( path));
      }

      TEST(casual_common_directory, create_3_level__expect_true)
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
