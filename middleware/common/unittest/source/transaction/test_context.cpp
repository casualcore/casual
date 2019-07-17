//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <common/unittest.h>

#include "common/exception/tx.h"


#include "common/transaction/context.h"


namespace casual
{
   namespace common
   {
      namespace transaction
      {

         TEST( casual_common_transaction_context, current__expect_null_transaction)
         {
            common::unittest::Trace trace;

            EXPECT_TRUE( Context::instance().current().trid.null());

         }

         TEST( casual_common_transaction_context, suspend__no_current_transaction__expect_TX_PROTOCOL_ERROR)
         {
            common::unittest::Trace trace;

            XID xid;

            EXPECT_THROW( Context::instance().suspend( &xid), exception::tx::Protocol);
         }

         TEST( casual_common_transaction_context, begin__expect_transaction)
         {
            common::unittest::Trace trace;

            EXPECT_NO_THROW( Context::instance().begin());
            EXPECT_TRUE( ! Context::instance().current().trid.null());
            EXPECT_NO_THROW( Context::instance().commit());
         }

         TEST( casual_common_transaction_context, two_begin__expect_TX_PROTOCOLL_ERROR)
         {
            common::unittest::Trace trace;


            EXPECT_NO_THROW( context().begin());
            EXPECT_THROW( context().begin(), exception::tx::Protocol);
            EXPECT_NO_THROW( context().rollback());
         }

         TEST( casual_common_transaction_context, begin_suspend_resume__rollback__expect_TX_OK)
         {
            common::unittest::Trace trace;

            XID xid;

            ASSERT_NO_THROW( Context::instance().begin());
            ASSERT_NO_THROW( Context::instance().suspend( &xid));
            EXPECT_TRUE( Context::instance().current().trid.null());
            ASSERT_NO_THROW( Context::instance().resume( &xid));
            ASSERT_NO_THROW( Context::instance().rollback());
         }

         TEST( casual_common_transaction_context, begin__10__suspend_begin_suspend_resume__rollback__expect_TX_OK)
         {
            common::unittest::Trace trace;

            // global...
            ASSERT_NO_THROW( Context::instance().begin());

            std::vector< XID> xids( 10);

            // start 10 new ones...
            for( auto& xid : xids)
            {
               ASSERT_NO_THROW( Context::instance().suspend( &xid));
               EXPECT_TRUE( Context::instance().current().trid.null());
               ASSERT_NO_THROW( Context::instance().begin());
            }

            for( auto& xid : xids)
            {
               EXPECT_NO_THROW( Context::instance().commit());
               ASSERT_NO_THROW( Context::instance().resume( &xid)) << "xid: " << xid;
               EXPECT_TRUE( Context::instance().current().trid == xid);
            }

            EXPECT_NO_THROW( Context::instance().rollback());
         }

         TEST( casual_common_transaction_context, no_xid__expect___pending_false)
         {
            common::unittest::Trace trace;

            EXPECT_TRUE( ! Context::instance().pending());
         }

         TEST( casual_common_transaction_context, begin___expect__pending_true)
         {
            common::unittest::Trace trace;

            Context::instance().begin();

            EXPECT_TRUE( Context::instance().pending());
            Context::instance().commit();
         }

         TEST( casual_common_transaction_context, join_extern_trid___expect__pending_false)
         {
            common::unittest::Trace trace;

            auto trid = transaction::id::create( process::Handle{ strong::process::id{ 1}, {}});

            Context::instance().join( trid);

            EXPECT_TRUE( ! Context::instance().pending());
            Context::instance().finalize( false);
         }


      } // transaction
   } // common
} // casual
