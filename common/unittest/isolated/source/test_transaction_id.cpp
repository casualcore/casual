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
      namespace local
      {
         transaction::ID transaction()
         {
            transaction::ID id = transaction::ID::create();

            auto& xid = id.xid();

            Uuid gtrid( "68f0bf5f-ff7f-0000-c02a-0a0001000000");
            Uuid btrid( "68f0bf5f-ff7f-0000-c02a-0a0001000001");

            gtrid.copy( reinterpret_cast< Uuid::uuid_type&>( xid.data));
            xid.gtrid_length = 16;

            auto bstart = std::begin( xid.data) + xid.gtrid_length;
            btrid.copy( reinterpret_cast< Uuid::uuid_type&>( bstart));
            xid.bqual_length = 16;

            return id;
         }

      } // locol
      TEST( casual_common_transaction_id, global_string)
      {
         transaction::ID id = local::transaction();

         EXPECT_TRUE( id.stringGlobal() == "68f0bf5f-ff7f-0000-c02a-0a0001000000") << id.stringGlobal();

      }
   } // common

} // casual
