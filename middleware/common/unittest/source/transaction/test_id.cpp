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
         const transaction::ID id{ gtrid, bqual, process::id()};

         // Check to see that the memory is exactly the same
         EXPECT_TRUE( algorithm::equal( id.global(), gtrid.range()));
         EXPECT_TRUE( algorithm::equal( id.branch(), bqual.range()));
      }


      TEST( casual_common_transaction_id, not_equal)
      {
         common::unittest::Trace trace;

         auto lhs = transaction::id::create();
         auto rhs = transaction::id::create();

         EXPECT_TRUE( lhs != rhs) << "lhs: " << lhs << '\n' << "rhs: " << rhs << '\n';

      }


      TEST( casual_common_transaction_id, owner)
      {
         common::unittest::Trace trace;

         auto trid = transaction::id::create();

         EXPECT_TRUE( trid.owner() == process::id()) << trace.compose( CASUAL_NAMED_VALUE( trid), '\n', CASUAL_NAMED_VALUE( process::handle()));

      }

      TEST( casual_common_transaction_id, equal)
      {
         common::unittest::Trace trace;

         auto lhs = transaction::id::create();
         auto rhs = lhs;

         EXPECT_TRUE( lhs == rhs) << "lhs: " << lhs << '\n' << "rhs: " << rhs << '\n';
         EXPECT_TRUE( lhs.owner() == rhs.owner());

      }


      TEST( casual_common_transaction_id, global_id)
      {
         common::unittest::Trace trace;

         auto gtrid = uuid::make();
         auto bqual = uuid::make();
         const transaction::ID id{ gtrid, bqual, process::id()};

         EXPECT_TRUE( algorithm::equal( gtrid.range(), id.global())) << "id: " << id << " - gtrid: " << CASUAL_NAMED_VALUE( gtrid);

      }

      TEST( casual_common_transaction_id, branch_id)
      {
         common::unittest::Trace trace;

         auto gtrid = uuid::make();
         auto bqual = uuid::make();
         const transaction::ID id{ gtrid, bqual, process::id()};

         EXPECT_TRUE( algorithm::equal( bqual.range(), id.branch())) << "id: " << id << " - char_gtrid: " << CASUAL_NAMED_VALUE( bqual);
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
