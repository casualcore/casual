//!
//! casual
//!

#include <common/unittest.h>

#include "common/transaction/id.h"
#include "common/uuid.h"

namespace casual
{
   namespace common
   {


      TEST( casual_common_transaction_id, ostream)
      {
         common::unittest::Trace trace;

         const Uuid gtrid = uuid::make();

         transaction::ID id{ gtrid, gtrid, { strong::process::id{ 0}, strong::ipc::id{ 0}}};

         std::ostringstream stream;
         stream << id;

         std::ostringstream expected;
         expected << gtrid << ':' << gtrid << ':' << transaction::ID::cCasual << ":0:0";


         EXPECT_TRUE( stream.str() == expected.str()) << "stream: " << stream.str() << std::endl << "expected: " << expected.str() << std::endl;
      }

      TEST( casual_common_transaction_id, generic_string)
      {
         common::unittest::Trace trace;

         transaction::ID id{ uuid::make(), uuid::make(), {  strong::process::id{ 0}, strong::ipc::id{ 0}}};

         std::ostringstream stream;
         stream << id;

         auto trid = common::string::split( stream.str(), ':');

         ASSERT_TRUE( trid.size() == 5);
         EXPECT_TRUE( trid[ 0].size() == 32);
         EXPECT_TRUE( trid[ 1].size() == 32);

         EXPECT_TRUE( trid[ 3].size() == 1);
         EXPECT_TRUE( trid[ 4].size() == 1);
      }

      TEST( casual_common_transaction_id, uuid_constructor)
      {
         common::unittest::Trace trace;

         auto gtrid = uuid::make();
         auto bqual = uuid::make();
         const transaction::ID id{ gtrid, bqual, process::handle()};

         auto size = sizeof( Uuid::uuid_type);

         //
         // Check to see that the memory is exactly the same
         //
         EXPECT_TRUE( memcmp( id.xid.data, gtrid.get(), size) == 0);
         EXPECT_TRUE( memcmp( id.xid.data + size, bqual.get(), size) == 0);

      }


      TEST( casual_common_transaction_id, not_equal)
      {
         common::unittest::Trace trace;

         auto lhs = transaction::ID::create();
         auto rhs = transaction::ID::create();

         EXPECT_TRUE( lhs != rhs) << "lhs: " << lhs << std::endl << "rhs: " << rhs << std::endl;

      }

      TEST( casual_common_transaction_id, equal_to_xid__expect_true)
      {
         common::unittest::Trace trace;

         auto trid = transaction::ID::create();

         EXPECT_TRUE( trid.xid == trid.xid) << "trid: " << trid << std::endl;
      }

      TEST( casual_common_transaction_id, owner)
      {
         common::unittest::Trace trace;

         auto trid = transaction::ID::create();

         EXPECT_TRUE( trid.owner() == process::handle()) << "trid.owner(): " << trid.owner() << std::endl << "process::handle()" << process::handle() << std::endl;

      }

      TEST( casual_common_transaction_id, equal)
      {
         common::unittest::Trace trace;

         auto lhs = transaction::ID::create();
         auto rhs = lhs;

         EXPECT_TRUE( lhs == rhs) << "lhs: " << lhs << std::endl << "rhs: " << rhs << std::endl;
         EXPECT_TRUE( lhs.owner() == rhs.owner());

      }



      TEST( casual_common_transaction_id, move)
      {
         common::unittest::Trace trace;

         auto id = transaction::ID::create();

         transaction::ID moved{ std::move( id)};

         EXPECT_TRUE( id.null());
         EXPECT_FALSE( moved.null());
      }

      TEST( casual_common_transaction_id, global_id)
      {
         common::unittest::Trace trace;

         auto gtrid = uuid::make();
         auto bqual = uuid::make();
         const transaction::ID id{ gtrid, bqual, process::handle()};

         char char_gtrid[ sizeof( Uuid::uuid_type)];
         algorithm::copy( gtrid.get(), std::begin( char_gtrid));

         EXPECT_TRUE( algorithm::equal( char_gtrid, transaction::global( id))) << "global: " << transaction::global( id) << " - char_gtrid: " << range::make( char_gtrid);
         EXPECT_TRUE( algorithm::equal( char_gtrid, transaction::global( id.xid)));

      }

      TEST( casual_common_transaction_id, branch_id)
      {
         common::unittest::Trace trace;

         auto gtrid = uuid::make();
         auto bqual = uuid::make();
         const transaction::ID id{ gtrid, bqual, process::handle()};

         char char_bqual[ sizeof( Uuid::uuid_type)];
         algorithm::copy( bqual.get(), std::begin( char_bqual));

         EXPECT_TRUE( algorithm::equal( char_bqual, transaction::branch( id))) << "branch: " << transaction::branch( id) << " - char_gtrid: " << range::make( char_bqual);
         EXPECT_TRUE( algorithm::equal( char_bqual, transaction::branch( id.xid)));
      }

   } // common

} // casual
