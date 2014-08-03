//!
//! test_transaction_id.cpp
//!
//! Created on: Nov 2, 2013
//!     Author: Lazan
//!

#include <gtest/gtest.h>

#include "common/transaction_id.h"
#include "common/uuid.h"

namespace casual
{
   namespace common
   {


      TEST( casual_common_transaction_id, global_string)
      {
         const Uuid gtrid = Uuid::make();

         transaction::ID id{ gtrid, gtrid};


         EXPECT_TRUE( id.stringGlobal() == gtrid.string()) << "id: " << id.stringGlobal() << std::endl << "uuid: " << gtrid.string() << std::endl;
      }

      TEST( casual_common_transaction_id, generic_string)
      {
         transaction::ID id{ Uuid::make(), Uuid::make()};
         auto gtrid = id.stringGlobal();

         id.xid().gtrid_length = 8;


         auto half_gtrid = id.stringGlobal();
         EXPECT_TRUE( half_gtrid.size() == 17) << "gtrid.....: " << gtrid << std::endl << "half_gtrid: " << half_gtrid << std::endl;
      }

      TEST( casual_common_transaction_id, uuid_constructor)
      {
         auto gtrid = Uuid::make();
         auto bqual = Uuid::make();
         const transaction::ID id{ gtrid, bqual};

         auto size = sizeof( Uuid::uuid_type);

         //
         // Check to see that the memory is exactly the same
         //
         EXPECT_TRUE( memcmp( id.xid().data, gtrid.get(), size) == 0);
         EXPECT_TRUE( memcmp( id.xid().data + size, bqual.get(), size) == 0);

         EXPECT_TRUE( id.stringGlobal() == gtrid.string());
         EXPECT_TRUE( id.stringBranch() == bqual.string());

      }


      TEST( casual_common_transaction_id, not_equal)
      {
         auto lhs = transaction::ID::create();
         auto rhs = transaction::ID::create();

         EXPECT_TRUE( lhs != rhs) << "lhs: " << lhs.stringGlobal() << std::endl << "rhs: " << rhs.stringGlobal() << std::endl;

      }

      TEST( casual_common_transaction_id, equal)
      {
         auto gtrid = Uuid::make();
         transaction::ID lhs{ gtrid, gtrid};
         transaction::ID rhs{ gtrid, gtrid};

         EXPECT_TRUE( lhs == rhs) << "lhs: " << lhs.stringGlobal() << std::endl << "rhs: " << rhs.stringGlobal() << std::endl;

      }


      TEST( casual_common_transaction_id, move)
      {
         auto id = transaction::ID::create();
         auto gtrid = id.stringGlobal();

         transaction::ID moved{ std::move( id)};

         EXPECT_TRUE( id.null());
         EXPECT_FALSE( moved.null());
         EXPECT_TRUE( moved.stringGlobal() == gtrid) << "moved: " << moved.stringGlobal() << std::endl << "gtrid: " << gtrid << std::endl;

      }

      TEST( casual_common_transaction_id, global_id)
      {
         auto gtrid = Uuid::make();
         auto bqual = Uuid::make();
         const transaction::ID id{ gtrid, bqual};

         char char_gtrid[ sizeof( Uuid::uuid_type)];
         range::copy( gtrid.get(), std::begin( char_gtrid));

         EXPECT_TRUE( range::equal( char_gtrid, transaction::global( id))) << "global: " << transaction::global( id) << " - char_gtrid: " << range::make( char_gtrid);
         EXPECT_TRUE( range::equal( char_gtrid, transaction::global( id.xid())));

      }

      TEST( casual_common_transaction_id, branch_id)
      {
         auto gtrid = Uuid::make();
         auto bqual = Uuid::make();
         const transaction::ID id{ gtrid, bqual};

         char char_bqual[ sizeof( Uuid::uuid_type)];
         range::copy( bqual.get(), std::begin( char_bqual));

         EXPECT_TRUE( range::equal( char_bqual, transaction::branch( id))) << "branch: " << transaction::branch( id) << " - char_gtrid: " << range::make( char_bqual);
         EXPECT_TRUE( range::equal( char_bqual, transaction::branch( id.xid())));
      }

   } // common

} // casual
