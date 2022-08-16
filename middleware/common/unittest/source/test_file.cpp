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

      TEST( common_file_find_pattern, existing_files)
      {
         common::unittest::Trace trace;

         auto pattern = std::filesystem::path{ __FILE__}.parent_path() / "test_*.cpp";

         auto paths = common::file::find( pattern.string());

         EXPECT_TRUE( paths.size() > 1) << trace.compose( "paths: ", paths);
      }

      TEST( common_file_find_pattern, specific_file)
      {
         common::unittest::Trace trace;
         
         auto paths = common::file::find( __FILE__);
         EXPECT_TRUE( paths.size() == 1) << trace.compose( "paths: ", paths);
         EXPECT_TRUE( paths.at( 0) == __FILE__) << trace.compose( "paths: ", paths);
      }

      TEST( common_file_find_pattern, nonexistent_path)
      {
         common::unittest::Trace trace;

         auto pattern = common::file::name::unique( "/a/b/c/", "/*");
         
         auto paths = common::file::find( pattern);
         EXPECT_TRUE( paths.empty()) << trace.compose( "paths: ", paths);
      }

      TEST( common_file_find_pattern, empty_pattern__expect_no_found)
      {
         common::unittest::Trace trace;
         
         auto paths = common::file::find( "");

         EXPECT_TRUE( paths.empty()) << trace.compose( "paths: ", paths);
      }

      TEST( common_file_find_pattern, same_pattern_twice__expect_unique_paths)
      {
         common::unittest::Trace trace;

         auto pattern = std::filesystem::path{ __FILE__}.parent_path() / "test_*.cpp";
         
         auto paths = common::file::find( { pattern.string(), pattern.string()});

         EXPECT_TRUE( paths.size() > 1) << trace.compose( "paths: ", paths);
         EXPECT_TRUE( algorithm::is::unique( paths)) << trace.compose( "paths: ", paths);
         EXPECT_TRUE( common::file::find( pattern.string()) == paths) << trace.compose( "paths: ", paths);
      }


      TEST( common_file_find_pattern, two_patterns__expect_preserved_order_between_the_patterns)
      {
         common::unittest::Trace trace;

         unittest::directory::temporary::Scoped dir;
         file::output::Append a{ dir.path() / "a_foo.yaml"};
         file::output::Append b{ dir.path() / "b_foo.yaml"};
         
         auto paths = common::file::find( { dir.path() / "b*.yaml", dir.path() / "a*.yaml"});
         ASSERT_TRUE( paths.size() == 2) << trace.compose( "paths: ", paths);
         EXPECT_TRUE( paths.at( 0) == b.path()) << trace.compose( "paths: ", paths);
         EXPECT_TRUE( paths.at( 1) == a.path()) << trace.compose( "paths: ", paths);
      }

      TEST( common_file_find_pattern, two_files__expect_sorted_order)
      {
         common::unittest::Trace trace;

         unittest::directory::temporary::Scoped dir;
         file::output::Append b{ dir.path() / "b_foo.yaml"};
         file::output::Append a{ dir.path() / "a_foo.yaml"};
         
         auto paths = common::file::find( ( dir.path() / "*.yaml").string());
         ASSERT_TRUE( paths.size() == 2) << trace.compose( "paths: ", paths);
         EXPECT_TRUE( paths.at( 0) == a.path()) << trace.compose( "paths: ", paths);
         EXPECT_TRUE( paths.at( 1) == b.path()) << trace.compose( "paths: ", paths);
      }

   } // common
} // casual
