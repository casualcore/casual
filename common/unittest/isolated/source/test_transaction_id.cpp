//!
//! test_transaction_id.cpp
//!
//! Created on: Nov 2, 2013
//!     Author: Lazan
//!

#include <gtest/gtest.h>

#include "common/transaction/id.h"
#include "common/uuid.h"

namespace casual
{
   namespace common
   {


      TEST( casual_common_transaction_id, ostream)
      {
         const Uuid gtrid = Uuid::make();

         transaction::ID id{ gtrid, gtrid, { 0, 0}};

         std::ostringstream stream;
         stream << id;

         std::ostringstream expected;
         expected << gtrid << ':' << gtrid << ":0:0";


         EXPECT_TRUE( stream.str() == expected.str()) << "stream: " << stream.str() << std::endl << "expected: " << expected.str() << std::endl;
      }

      TEST( casual_common_transaction_id, generic_string)
      {
         transaction::ID id{ Uuid::make(), Uuid::make(), { 0, 0}};
         id.xid.gtrid_length = 8;
         id.xid.bqual_length = 8;

         std::ostringstream stream;
         stream << id;

         auto gtrid = stream.str();

         EXPECT_TRUE( gtrid.size() == 17 * 2 + 1 + 4) << "id: " << id << std::endl;
      }

      TEST( casual_common_transaction_id, uuid_constructor)
      {
         auto gtrid = Uuid::make();
         auto bqual = Uuid::make();
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
         auto lhs = transaction::ID::create();
         auto rhs = transaction::ID::create();

         EXPECT_TRUE( lhs != rhs) << "lhs: " << lhs << std::endl << "rhs: " << rhs << std::endl;

      }

      TEST( casual_common_transaction_id, owner)
      {
         auto trid = transaction::ID::create();

         EXPECT_TRUE( trid.owner() == process::handle()) << "trid.owner(): " << trid.owner() << std::endl << "process::handle()" << process::handle() << std::endl;

      }

      TEST( casual_common_transaction_id, equal)
      {
         auto lhs = transaction::ID::create();
         auto rhs = lhs;

         EXPECT_TRUE( lhs == rhs) << "lhs: " << lhs << std::endl << "rhs: " << rhs << std::endl;
         EXPECT_TRUE( lhs.owner() == rhs.owner());

      }



      TEST( casual_common_transaction_id, move)
      {
         auto id = transaction::ID::create();

         transaction::ID moved{ std::move( id)};

         EXPECT_TRUE( id.null());
         EXPECT_FALSE( moved.null());
      }

      TEST( casual_common_transaction_id, global_id)
      {
         auto gtrid = Uuid::make();
         auto bqual = Uuid::make();
         const transaction::ID id{ gtrid, bqual, process::handle()};

         char char_gtrid[ sizeof( Uuid::uuid_type)];
         range::copy( gtrid.get(), std::begin( char_gtrid));

         EXPECT_TRUE( range::equal( char_gtrid, transaction::global( id))) << "global: " << transaction::global( id) << " - char_gtrid: " << range::make( char_gtrid);
         EXPECT_TRUE( range::equal( char_gtrid, transaction::global( id.xid)));

      }

      TEST( casual_common_transaction_id, branch_id)
      {
         auto gtrid = Uuid::make();
         auto bqual = Uuid::make();
         const transaction::ID id{ gtrid, bqual, process::handle()};

         char char_bqual[ sizeof( Uuid::uuid_type)];
         range::copy( bqual.get(), std::begin( char_bqual));

         EXPECT_TRUE( range::equal( char_bqual, transaction::branch( id))) << "branch: " << transaction::branch( id) << " - char_gtrid: " << range::make( char_bqual);
         EXPECT_TRUE( range::equal( char_bqual, transaction::branch( id.xid)));
      }

   } // common

} // casual
