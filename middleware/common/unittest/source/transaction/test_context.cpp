//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <common/unittest.h>

#include "common/code/tx.h"


#include "common/transaction/context.h"


namespace casual
{
   namespace common
   {
      namespace transaction
      {

         TEST( common_transaction_context, current__expect_null_transaction)
         {
            common::unittest::Trace trace;

            EXPECT_TRUE( context().current().trid.null());
         }



         TEST( common_transaction_context, suspend__no_current_transaction__expect_TX_PROTOCOL_ERROR)
         {
            common::unittest::Trace trace;

            XID xid;

            EXPECT_TRUE( context().suspend( &xid) == code::tx::protocol);
         }

         TEST( common_transaction_context, begin__expect_transaction)
         {
            common::unittest::Trace trace;

            EXPECT_EQ( context().begin(), code::tx::ok);
            EXPECT_TRUE( ! context().current().trid.null());
            EXPECT_EQ( context().commit(), code::tx::ok);
         }

         TEST( common_transaction_context, two_begin__expect_TX_PROTOCOLL_ERROR)
         {
            common::unittest::Trace trace;

            EXPECT_EQ( context().begin(), code::tx::ok);
            EXPECT_EQ( context().begin(), code::tx::protocol);
            EXPECT_EQ( context().rollback(), code::tx::ok);
         }

         TEST( common_transaction_context, begin_suspend_resume__rollback__expect_TX_OK)
         {
            common::unittest::Trace trace;

            XID xid;

            EXPECT_EQ( context().begin(), code::tx::ok);
            ASSERT_EQ( context().suspend( &xid), code::tx::ok);
            EXPECT_TRUE( context().current().trid.null());
            ASSERT_EQ( context().resume( &xid), code::tx::ok);
            ASSERT_EQ( context().rollback(), code::tx::ok);
         }

         TEST( common_transaction_context, begin__10__suspend_begin_suspend_resume__rollback__expect_TX_OK)
         {
            common::unittest::Trace trace;

            // global...
            ASSERT_EQ( context().begin(), code::tx::ok);

            std::vector< XID> xids( 10);

            // start 10 new ones...
            for( auto& xid : xids)
            {
               ASSERT_EQ( context().suspend( &xid), code::tx::ok);
               EXPECT_TRUE( context().current().trid.null());
               ASSERT_EQ( context().begin(), code::tx::ok);
            }

            for( auto& xid : xids)
            {
               EXPECT_EQ( context().commit(), code::tx::ok);
               ASSERT_EQ( context().resume( &xid), code::tx::ok) << "xid: " << xid;
               EXPECT_TRUE( context().current().trid == xid);
            }

            EXPECT_EQ( context().rollback(), code::tx::ok);
         }

         TEST( common_transaction_context, no_xid__expect___pending_false)
         {
            common::unittest::Trace trace;

            EXPECT_TRUE( ! context().pending());
         }

         TEST( common_transaction_context, begin___expect__pending_true)
         {
            common::unittest::Trace trace;

            ASSERT_EQ( context().begin(), code::tx::ok);

            EXPECT_TRUE( context().pending());
            EXPECT_EQ( context().commit(), code::tx::ok);
         }

         TEST( common_transaction_context, join_extern_trid___expect__pending_false)
         {
            common::unittest::Trace trace;

            auto trid = transaction::id::create( process::Handle{ strong::process::id{ 1}, {}});

            ASSERT_NO_THROW( context().join( trid));

            EXPECT_TRUE( ! context().pending());
            
            context().finalize( false);
         }

         TEST( common_transaction_context, begin_commit__expect_context_empty)
         {
            common::unittest::Trace trace;

            EXPECT_EQ( context().begin(), code::tx::ok);
            EXPECT_TRUE( ! context().empty());
            EXPECT_EQ( context().commit(), code::tx::ok);
            EXPECT_TRUE( context().empty());
            
         }

      } // transaction
   } // common
} // casual
