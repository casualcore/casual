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

         // this will trigger use-after-move
         EXPECT_TRUE( id.null()); // NOLINT
         EXPECT_TRUE( ! moved.null());
      }

      TEST( casual_common_transaction_id, global_id)
      {
         common::unittest::Trace trace;

         auto gtrid = uuid::make();
         auto bqual = uuid::make();
         const transaction::ID id{ gtrid, bqual, process::handle()};

         EXPECT_TRUE( algorithm::equal( gtrid.range(), transaction::id::range::global( id))) << "id: " << id << " - gtrid: " << CASUAL_NAMED_VALUE( gtrid);
         EXPECT_TRUE( algorithm::equal( gtrid.range(), transaction::id::range::global( id.xid)));

      }

      TEST( casual_common_transaction_id, branch_id)
      {
         common::unittest::Trace trace;

         auto gtrid = uuid::make();
         auto bqual = uuid::make();
         const transaction::ID id{ gtrid, bqual, process::handle()};

         EXPECT_TRUE( algorithm::equal( bqual.range(), transaction::id::range::branch( id))) << "id: " << id << " - char_gtrid: " << CASUAL_NAMED_VALUE( bqual);
         EXPECT_TRUE( algorithm::equal( bqual.range(), transaction::id::range::branch( id.xid)));
      }

      TEST( common_transaction_global_id, istream_operator)
      {
         common::unittest::Trace trace;

         // longer hex gtrid than 64 bytes
         const std::string hex_id{ "00000000000000000000ffff0a82176e877949d1684dda18002d377c756b6f2d72616b75722d363864376336356437632d6d35"};
         
         std::istringstream stream{ hex_id};

         transaction::global::ID gtrid;
         stream >> gtrid;

         EXPECT_TRUE( gtrid.range().size() == hex_id.size() / 2);
         EXPECT_TRUE( transcode::hex::encode( gtrid.range()) == hex_id);
      }

      TEST( common_transaction_global_id, istream_operator_invalid_hex)
      {
         common::unittest::Trace trace;

         auto deserialize = []( const std::string& hex_id)
         {
            std::istringstream stream{ hex_id};

            transaction::global::ID gtrid;
            stream >> gtrid;

            return gtrid;
         };

         // uneven length
         EXPECT_CODE( deserialize( "12345"), code::casual::invalid_argument) << "expected invalid hex gtrid to throw";
         // invalid characters
         EXPECT_CODE( deserialize( "1234gn"), code::casual::invalid_argument);
         // too long
         EXPECT_CODE( deserialize( std::string( 129, 'a')), code::casual::invalid_argument) << "expected invalid hex gtrid to throw";

      }
 

   } // common

} // casual
