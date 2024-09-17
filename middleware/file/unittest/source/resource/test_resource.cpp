//!
//! Copyright (c) 2024, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#define CASUAL_NO_XATMI_UNDEFINE

#include "common/unittest.h"
#include "common/unittest/file.h"
#include "domain/unittest/manager.h"

#include "common/transaction/context.h"

#include "file/api/file.h"

#include "file/resource/unittest/utility.h"

#include "casual/tx/code.h"

#include <fstream>


namespace casual
{
   namespace file::resource
   {

      namespace local
      {
         namespace
         {
            auto domain()
            {
               constexpr auto servers = R"(
domain: 
   name: file-domain

   groups: 
      - name: base
      - name: file
        dependencies: [ base]

   servers:
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager
        memberships: [ base]
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/casual-transaction-manager
        memberships: [ base]
      - path: bin/casual-file-manager
        memberships: [ file]
)";

               return domain::unittest::manager( servers);
            }
         } // 
      } // local


      TEST( casual_file_resource, update_file__commit__expect_updated_file)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         const auto path = common::unittest::file::temporary::name( ".txt");

         const std::string_view afore{ "aaa"};
         const std::string_view after{ "xxx"};

         std::ofstream{ path} << afore;

         {
            EXPECT_EQ( common::transaction::context().begin(), common::code::tx::ok);
            const auto reserved = file::blocking::reserve( path);
            std::ofstream{ reserved} << after;
            EXPECT_EQ( common::transaction::context().commit(), common::code::tx::ok);
         }

         std::string result;
         std::ifstream{ path} >> result;
         EXPECT_EQ( after, result) << result;
      }

      TEST( casual_file_resource, update_file__rollback__expect_earlier_file)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         const auto path = common::unittest::file::temporary::name( ".txt");

         const std::string_view afore{ "aaa"};
         const std::string_view after{ "xxx"};

         std::ofstream{ path} << afore;

         {
            EXPECT_EQ( common::transaction::context().begin(), common::code::tx::ok);
            const auto reserved = file::blocking::reserve( path);
            std::ofstream{ reserved} << after;
            EXPECT_EQ( common::transaction::context().rollback(), common::code::tx::ok);
         }

         std::string result;
         std::ifstream{ path} >> result;
         EXPECT_EQ( afore, result) << result;
      }

      TEST( casual_file_resource, delete_file__commit__expect_deleted_file)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         const auto path = common::unittest::file::temporary::content( ".txt", "aaa");

         {
            EXPECT_EQ( common::transaction::context().begin(), common::code::tx::ok);
            const auto reserved = file::blocking::reserve( path);
            std::filesystem::remove( reserved);
            EXPECT_EQ( common::transaction::context().commit(), common::code::tx::ok);
         }

         EXPECT_FALSE( std::filesystem::exists( path)) << path;
      }

      TEST( casual_file_resource, delete_file__rollback__expect_initial_file)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         const auto path = common::unittest::file::temporary::content( ".txt", "aaa");

         {
            EXPECT_EQ( common::transaction::context().begin(), common::code::tx::ok);
            const auto reserved = file::blocking::reserve( path);
            std::filesystem::remove( reserved);
            EXPECT_EQ( common::transaction::context().rollback(), common::code::tx::ok);
         }

         EXPECT_TRUE( std::filesystem::exists( path)) << path;
      }

      TEST( casual_file_resource, rename_file__commit__expect_renamed_file)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         const auto source = common::unittest::file::temporary::name( ".txt");
         const auto target = common::unittest::file::temporary::name( ".txt");

         std::ofstream{ source} << "aaa";

         {
            EXPECT_EQ( common::transaction::context().begin(), common::code::tx::ok);
            const auto first = file::blocking::reserve( source);
            const auto other = file::blocking::reserve( target);
            std::filesystem::rename( first, other);
            EXPECT_EQ( common::transaction::context().commit(), common::code::tx::ok);
         }

         EXPECT_FALSE( std::filesystem::exists( source)) << source;
         EXPECT_TRUE( std::filesystem::exists( target)) << target;
      }

      TEST( casual_file_resource, rename_file__rollback__expect_initial_file)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         const auto source = common::unittest::file::temporary::name( ".txt");
         const auto target = common::unittest::file::temporary::name( ".txt");

         std::ofstream{ source} << "aaa";

         {
            EXPECT_EQ( common::transaction::context().begin(), common::code::tx::ok);
            const auto first = file::blocking::reserve( source);
            const auto other = file::blocking::reserve( target);
            std::filesystem::rename( first, other);
            EXPECT_EQ( common::transaction::context().rollback(), common::code::tx::ok);
         }

         EXPECT_TRUE( std::filesystem::exists( source)) << source;
         EXPECT_FALSE( std::filesystem::exists( target)) << target;
      }

      TEST( casual_file_resource, intact_file__commit__expect_initial_file)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         const auto original = common::unittest::file::temporary::content( ".txt", "abc");

         EXPECT_EQ( common::transaction::context().begin(), common::code::tx::ok);
         const auto reserved = file::blocking::reserve( original);
         EXPECT_EQ( common::transaction::context().commit(), common::code::tx::ok);

         EXPECT_TRUE( std::filesystem::exists( original)) << original;
         EXPECT_FALSE( std::filesystem::exists( reserved)) << reserved;
      }

      TEST( casual_file_resource, intact_file__rollback__expect_initial_file)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         const auto original = common::unittest::file::temporary::content( ".txt", "abc");

         EXPECT_EQ( common::transaction::context().begin(), common::code::tx::ok);
         const auto reserved = file::blocking::reserve( original);
         EXPECT_EQ( common::transaction::context().rollback(), common::code::tx::ok);

         EXPECT_TRUE( std::filesystem::exists( original)) << original;
         EXPECT_FALSE( std::filesystem::exists( reserved)) << reserved;
      }

      TEST( casual_file_resource, reserve_without_transaction__expect_exception)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         const auto path = common::unittest::file::temporary::name( ".txt");

         EXPECT_THROW({
            const auto work = file::blocking::reserve( path);
         }, std::system_error);

         EXPECT_THROW({
            const auto work = file::non::blocking::reserve( path);
         }, std::system_error);
      }

      TEST( casual_file_resource, multiple_reserve_in_same_transaction__expect_no_block)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         const auto path = common::unittest::file::temporary::name( ".txt");

         EXPECT_EQ( common::transaction::context().begin(), common::code::tx::ok);
         const auto first = file::blocking::reserve( path);
         const auto other = file::blocking::reserve( path);
         EXPECT_EQ( common::transaction::context().rollback(), common::code::tx::ok);

         EXPECT_FALSE( first.empty());
         EXPECT_FALSE( other.empty());
         EXPECT_EQ( first, other);
      }

      TEST( casual_file_resource, multiple_non_blocking_reserve_in_same_transaction__expect_no_block)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         const auto path = common::unittest::file::temporary::name( ".txt");

         EXPECT_EQ( common::transaction::context().begin(), common::code::tx::ok);
         const auto first = file::blocking::reserve( path);
         const auto other = file::blocking::reserve( path);
         EXPECT_EQ( common::transaction::context().rollback(), common::code::tx::ok);

         EXPECT_FALSE( first.empty());
         EXPECT_FALSE( other.empty());
         EXPECT_EQ( first, other);
      }

      TEST( casual_file_resource, blocking_reserve_and_non_blocking_reserve_in_different_transactions__expect_correct_result)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         const auto path = common::unittest::file::temporary::name( ".txt");

         XID first;
         ASSERT_EQ( TX_OK, tx_begin());
         EXPECT_FALSE( file::blocking::reserve( path).empty());
         ASSERT_EQ( TX_OK, tx_suspend( &first));

         XID other;
         ASSERT_EQ( TX_OK, tx_begin());
         // non blocking failure
         EXPECT_FALSE( file::non::blocking::reserve( path).has_value());
         ASSERT_EQ( TX_OK, tx_suspend( &other));

         ASSERT_EQ( TX_OK, tx_resume( &first));
         ASSERT_EQ( TX_OK, tx_commit());

         ASSERT_EQ( TX_OK, tx_resume( &other));
         // non blocking success
         EXPECT_TRUE( file::non::blocking::reserve( path).has_value());

         ASSERT_EQ( TX_OK, tx_commit());
      }

      TEST( casual_file_resource, reserve_same_path_in_different_transactions__expect_manager_to_handle_working_and_pending)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         const auto path = common::unittest::file::temporary::name( ".txt");

         auto expect_state = []( const std::size_t working, const std::size_t pending)
         {
            const auto state = file::resource::unittest::state();
            EXPECT_EQ( working, state.working.size()) << state.working.size();
            EXPECT_EQ( pending, state.pending.size()) << state.pending.size();
         };

         expect_state( 0, 0);

         XID first;
         ASSERT_EQ( TX_OK, tx_begin());
         file::resource::unittest::blocking::reserve::send( path);
         ASSERT_EQ( TX_OK, tx_suspend( &first));

         expect_state( 1, 0);

         XID other;
         ASSERT_EQ( TX_OK, tx_begin());
         file::resource::unittest::blocking::reserve::send( path);
         ASSERT_EQ( TX_OK, tx_suspend( &other));

         expect_state( 1, 1);

         ASSERT_EQ( TX_OK, tx_resume( &first));
         EXPECT_FALSE( file::resource::unittest::blocking::reserve::receive().empty());
         ASSERT_EQ( TX_OK, tx_commit());

         expect_state( 1, 0);

         ASSERT_EQ( TX_OK, tx_resume( &other));
         EXPECT_FALSE( file::resource::unittest::blocking::reserve::receive().empty());
         ASSERT_EQ( TX_OK, tx_commit());

         expect_state( 0, 0);
      }


   } // file::resource

} // casual
