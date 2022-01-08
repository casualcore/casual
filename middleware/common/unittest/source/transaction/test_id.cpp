//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <common/unittest.h>

#include "common/transaction/id.h"
#include "common/uuid.h"

namespace casual
{
   namespace common
   {

      TEST( casual_common_transaction_id, uuid_constructor)
      {
         common::unittest::Trace trace;

         auto gtrid = uuid::make();
         auto bqual = uuid::make();
         const transaction::ID id{ gtrid, bqual, process::handle()};

         auto size = sizeof( Uuid::uuid_type);

         // Check to see that the memory is exactly the same
         EXPECT_TRUE( memcmp( id.xid.data, gtrid.get(), size) == 0);
         EXPECT_TRUE( memcmp( id.xid.data + size, bqual.get(), size) == 0);
      }


      TEST( casual_common_transaction_id, not_equal)
      {
         common::unittest::Trace trace;

         auto lhs = transaction::id::create();
         auto rhs = transaction::id::create();

         EXPECT_TRUE( lhs != rhs) << "lhs: " << lhs << '\n' << "rhs: " << rhs << '\n';

      }

      TEST( casual_common_transaction_id, equal_to_xid__expect_true)
      {
         common::unittest::Trace trace;

         auto trid = transaction::id::create();

         EXPECT_TRUE( trid.xid == trid.xid) << "trid: " << trid << '\n';
      }

      TEST( casual_common_transaction_id, owner)
      {
         common::unittest::Trace trace;

         auto trid = transaction::id::create();

         EXPECT_TRUE( trid.owner() == process::handle()) << trace.compose( CASUAL_NAMED_VALUE( trid), '\n', CASUAL_NAMED_VALUE( process::handle()));

      }

      TEST( casual_common_transaction_id, equal)
      {
         common::unittest::Trace trace;

         auto lhs = transaction::id::create();
         auto rhs = lhs;

         EXPECT_TRUE( lhs == rhs) << "lhs: " << lhs << '\n' << "rhs: " << rhs << '\n';
         EXPECT_TRUE( lhs.owner() == rhs.owner());

      }



      TEST( casual_common_transaction_id, move)
      {
         common::unittest::Trace trace;

         auto id = transaction::id::create();

         transaction::ID moved{ std::move( id)};

         EXPECT_TRUE( id.null()); // NOLINT
         EXPECT_TRUE( ! moved.null());
      }

      TEST( casual_common_transaction_id, global_id)
      {
         common::unittest::Trace trace;

         auto gtrid = uuid::make();
         auto bqual = uuid::make();
         const transaction::ID id{ gtrid, bqual, process::handle()};

         char char_gtrid[ sizeof( Uuid::uuid_type)];
         algorithm::copy( gtrid.get(), std::begin( char_gtrid));

         EXPECT_TRUE( algorithm::equal( char_gtrid, transaction::id::range::global( id))) << "global: " << transaction::id::range::global( id) << " - char_gtrid: " << view::binary::make( char_gtrid);
         EXPECT_TRUE( algorithm::equal( char_gtrid, transaction::id::range::global( id.xid)));

      }

      TEST( casual_common_transaction_id, branch_id)
      {
         common::unittest::Trace trace;

         auto gtrid = uuid::make();
         auto bqual = uuid::make();
         const transaction::ID id{ gtrid, bqual, process::handle()};

         char char_bqual[ sizeof( Uuid::uuid_type)];
         algorithm::copy( bqual.get(), std::begin( char_bqual));

         EXPECT_TRUE( algorithm::equal( char_bqual, transaction::id::range::branch( id))) << "branch: " << transaction::id::range::branch( id) << " - char_gtrid: " << view::binary::make( char_bqual);
         EXPECT_TRUE( algorithm::equal( char_bqual, transaction::id::range::branch( id.xid)));
      }

   } // common

} // casual
