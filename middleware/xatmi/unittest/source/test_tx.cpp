//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



//
// to be able to use 'raw' flags and codes
// since we undefine 'all' of them in common
//
#define CASUAL_NO_XATMI_UNDEFINE


#include "common/unittest.h"

#include "tx.h"

#include "common/transaction/context.h"

namespace casual
{
   namespace common
   {
      namespace transaction
      {

         TEST( xatmi_tx, tx_info__expect_0_transaction)
         {
            common::unittest::Trace trace;

            EXPECT_TRUE( tx_info( nullptr) == 0);
         }

         TEST( xatmi_tx, tx_suspend__no_current_transaction__expect_TX_PROTOCOL_ERROR)
         {
            common::unittest::Trace trace;

            XID xid;

            ASSERT_TRUE( tx_suspend( &xid) == TX_PROTOCOL_ERROR);
         }

         TEST( xatmi_tx, tx_begin__expect_transaction)
         {
            common::unittest::Trace trace;

            ASSERT_TRUE( tx_begin() == TX_OK);
            EXPECT_TRUE( tx_info( nullptr) == 1);
            EXPECT_TRUE( tx_commit() == TX_OK);
            EXPECT_TRUE( tx_info( nullptr) == 0);
         }

         TEST( xatmi_tx, two_tx_begin__expect_TX_PROTOCOLL_ERROR)
         {
            common::unittest::Trace trace;

            ASSERT_TRUE( tx_begin() == TX_OK);
            EXPECT_TRUE( tx_info( nullptr) == 1);
            EXPECT_TRUE( tx_begin() == TX_PROTOCOL_ERROR);
            EXPECT_TRUE( tx_rollback() == TX_OK);
            EXPECT_TRUE( tx_info( nullptr) == 0);
         }

         TEST( xatmi_tx, tx_begin_tx_suspend_tx_resume__tx_rollback__expect_TX_OK)
         {
            common::unittest::Trace trace;

            XID xid;

            ASSERT_TRUE( tx_begin() == TX_OK);
            ASSERT_TRUE( tx_suspend( &xid) == TX_OK);
            EXPECT_TRUE( tx_info( nullptr) == 0);
            EXPECT_TRUE( tx_resume( &xid) == TX_OK);
            EXPECT_TRUE( tx_rollback() == TX_OK);
         }

         TEST( xatmi_tx, tx_begin__10_tx_suspend_tx_begin___tx_commit_tx_resume__rollback__expect_TX_OK)
         {
            common::unittest::Trace trace;

            // global...
            ASSERT_TRUE( tx_begin() == TX_OK);

            std::vector< XID> xids( 10);

            // start 10 new ones...
            for( auto& xid : xids)
            {
               ASSERT_TRUE( tx_suspend( &xid) == TX_OK);
               EXPECT_TRUE( tx_info( nullptr) == 0);
               ASSERT_TRUE( tx_begin() == TX_OK);
            }

            for( auto& xid : xids)
            {
               EXPECT_TRUE( tx_commit() == TX_OK);
               EXPECT_TRUE( tx_info( nullptr) == 0);
               ASSERT_TRUE( tx_resume( &xid) == TX_OK) << "xid: " << xid;

               TXINFO txinfo;
               EXPECT_TRUE( tx_info( &txinfo) == 1);
               EXPECT_TRUE( txinfo.xid == xid);
            }

            EXPECT_TRUE( tx_rollback() == TX_OK);
         }

         TEST( xatmi_tx, tx_begin__10_tx_suspend_tx_begin__in_revers_order__tx_commit_tx_resume__rollback__expect_TX_OK)
         {
            common::unittest::Trace trace;

            // global...
            ASSERT_TRUE( tx_begin() == TX_OK);

            std::vector< XID> xids( 10);

            // start 10 new ones...
            for( auto& xid : xids)
            {
               ASSERT_TRUE( tx_suspend( &xid) == TX_OK);
               EXPECT_TRUE( tx_info( nullptr) == 0);
               ASSERT_TRUE( tx_begin() == TX_OK);
            }

            for( auto& xid : common::range::reverse( xids))
            {
               EXPECT_TRUE( tx_commit() == TX_OK);
               EXPECT_TRUE( tx_info( nullptr) == 0);
               ASSERT_TRUE( tx_resume( &xid) == TX_OK) << "xid: " << xid;

               TXINFO txinfo;
               EXPECT_TRUE( tx_info( &txinfo) == 1);
               EXPECT_TRUE( txinfo.xid == xid);
            }

            EXPECT_TRUE( tx_rollback() == TX_OK);
         }


         TEST( xatmi_tx, chained_control__tx_begin_tx_commit__expect_new_transaction)
         {
            common::unittest::Trace trace;

            ASSERT_EQ( tx_set_transaction_control( TX_CHAINED), TX_OK);

            EXPECT_EQ( tx_begin(), TX_OK);
            EXPECT_EQ( tx_info( nullptr), 1);
            EXPECT_EQ( tx_commit(), TX_OK);
            // expect a new transaction to start
            EXPECT_EQ( tx_info( nullptr), 1);

            EXPECT_EQ( tx_set_transaction_control( TX_UNCHAINED), TX_OK);

            EXPECT_EQ( tx_commit(), TX_OK);
            EXPECT_EQ( tx_info( nullptr), 0);
         }

         TEST( xatmi_tx, chained_control__tx_begin_tx_begin__expect_TX_PROTOCOLL_ERROR)
         {
            common::unittest::Trace trace;

            tx_set_transaction_control( TX_CHAINED);

            ASSERT_TRUE( tx_begin() == TX_OK);
            EXPECT_TRUE( tx_begin() == TX_PROTOCOL_ERROR);

            tx_set_transaction_control( TX_UNCHAINED);

            EXPECT_TRUE( tx_commit() == TX_OK);
         }

         TEST( xatmi_tx, stacked_control__tx_begin_tx_begin__expect_TX_OK)
         {
            common::unittest::Trace trace;

            tx_set_transaction_control( TX_STACKED);

            ASSERT_TRUE( tx_begin() == TX_OK);
            EXPECT_TRUE( tx_info( nullptr) == 1);
            EXPECT_TRUE( tx_begin() == TX_OK);
            EXPECT_TRUE( tx_info( nullptr) == 1);

            EXPECT_TRUE( tx_commit() == TX_OK);
            EXPECT_TRUE( tx_info( nullptr) == 1);
            EXPECT_TRUE( tx_commit() == TX_OK);

            EXPECT_TRUE( tx_info( nullptr) == 0);
         }

         TEST( xatmi_tx, stacked_control__tx_begin_tx_begin_tx_suspend__expect_TX_OK)
         {
            common::unittest::Trace trace;

            tx_set_transaction_control( TX_STACKED);

            ASSERT_TRUE( tx_begin() == TX_OK);
            EXPECT_TRUE( tx_info( nullptr) == 1);
            EXPECT_TRUE( tx_begin() == TX_OK);
            EXPECT_TRUE( tx_info( nullptr) == 1);

            XID xid;
            ASSERT_TRUE( tx_suspend( &xid) == TX_OK);
            EXPECT_TRUE( tx_info( nullptr) == 0);
            EXPECT_TRUE( tx_resume( &xid) == TX_OK);

            EXPECT_TRUE( tx_commit() == TX_OK);
            EXPECT_TRUE( tx_info( nullptr) == 1);
            EXPECT_TRUE( tx_commit() == TX_OK);

            EXPECT_TRUE( tx_info( nullptr) == 0);
         }


      } // transaction
   } // common
} // casual

