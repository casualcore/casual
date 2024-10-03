//!
//! Copyright (c) 2024, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"
#include "common/unittest/file.h"

#include "file/resource/logic.h"



#include <fstream>


namespace casual
{
   namespace file::resource
   {
      TEST( casual_file_resource_logic, update_file__perform__expect_updated_file)
      {
         common::unittest::Trace trace;

         const auto trid = common::transaction::id::create();

         const std::string_view afore{ "aaa"};
         const std::string_view after{ "xxx"};

         const auto temp = common::unittest::file::temporary::name( ".txt");

         std::ofstream{ temp} << afore;

         const auto path = file::resource::acquire( trid, temp);

         std::ofstream{ path} << after;

         // commit
         file::resource::perform( trid);

         std::string result;

         std::ifstream{ temp} >> result;

         EXPECT_EQ( after, result) << result;
      }

      TEST( casual_file_resource_logic, update_file__restore__expect_earlier_file)
      {
         common::unittest::Trace trace;

         const auto trid = common::transaction::id::create();

         const std::string_view afore{ "aaa"};
         const std::string_view after{ "xxx"};

         const auto temp = common::unittest::file::temporary::name( ".txt");

         std::ofstream{ temp} << afore;

         const auto path = file::resource::acquire( trid, temp);

         std::ofstream{ path} << after;

         // rollback
         file::resource::restore( trid);

         std::string result;

         std::ifstream{ temp} >> result;

         EXPECT_EQ( afore, result) << result;
      }

      TEST( casual_file_resource_logic, delete_file__perform__expect_deleted_file)
      {
         common::unittest::Trace trace;

         const auto trid = common::transaction::id::create();


         const auto temp = common::unittest::file::temporary::content( ".txt", "aaa");

         const auto path = file::resource::acquire( trid, temp);

         std::filesystem::remove( path);

         // commit
         file::resource::perform( trid);

         EXPECT_FALSE( std::filesystem::exists( temp)) << temp;
      }

      TEST( casual_file_resource_logic, delete_file__restore__expect_initial_file)
      {
         common::unittest::Trace trace;

         const auto trid = common::transaction::id::create();

         const auto temp = common::unittest::file::temporary::content( ".txt", "aaa");

         const auto path = file::resource::acquire( trid, temp);

         std::filesystem::remove( path);

         // rollback
         file::resource::restore( trid);

         EXPECT_TRUE( std::filesystem::exists( temp)) << temp;
      }

      TEST( casual_file_resource_logic, rename_file__perform__expect_renamed_file)
      {
         common::unittest::Trace trace;

         const auto trid = common::transaction::id::create();

         const auto source = common::unittest::file::temporary::name( ".txt");
         const auto target = common::unittest::file::temporary::name( ".txt");

         std::ofstream{ source} << "abc";

         std::filesystem::rename( file::resource::acquire( trid, source), file::resource::acquire( trid, target));

         // commit
         file::resource::perform( trid);

         EXPECT_FALSE( std::filesystem::exists( source)) << source;
         EXPECT_TRUE( std::filesystem::exists( target)) << target;
      }

      TEST( casual_file_resource_logic, rename_file__restore__expect_initial_file)
      {
         common::unittest::Trace trace;

         const auto trid = common::transaction::id::create();

         const auto source = common::unittest::file::temporary::name( ".txt");
         const auto target = common::unittest::file::temporary::name( ".txt");

         std::ofstream{ source} << "abc";

         std::filesystem::rename( file::resource::acquire( trid, source), file::resource::acquire( trid, target));

         // rollback
         file::resource::restore( trid);

         EXPECT_TRUE( std::filesystem::exists( source)) << source;
         EXPECT_FALSE( std::filesystem::exists( target)) << target;
      }

      TEST( casual_file_resource_logic, intact_file__perform__expect_initial_file)
      {
         common::unittest::Trace trace;

         const auto trid = common::transaction::id::create();

         const auto original = common::unittest::file::temporary::content( ".txt", "abc");

         const auto acquired = file::resource::acquire( trid, original);

         // commit
         file::resource::perform( trid);

         EXPECT_TRUE( std::filesystem::exists( original)) << original;
         EXPECT_FALSE( std::filesystem::exists( acquired)) << acquired;
      }

      TEST( casual_file_resource_logic, intact_file__restore__expect_initial_file)
      {
         common::unittest::Trace trace;

         const auto trid = common::transaction::id::create();

         const auto original = common::unittest::file::temporary::content( ".txt", "abc");

         const auto acquired = file::resource::acquire( trid, original);

         // rollback
         file::resource::restore( trid);

         EXPECT_TRUE( std::filesystem::exists( original)) << original;
         EXPECT_FALSE( std::filesystem::exists( acquired)) << acquired;
      }
   } // file::resource

} // casual