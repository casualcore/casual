//!
//! casual
//!

#include "common/unittest.h"

#include "tx.h"

#include "common/mockup/domain.h"


#include "common/transaction/context.h"

namespace casual
{
   namespace common
   {
      namespace transaction
      {

         namespace local
         {
            namespace
            {

               struct Domain
               {
                  //
                  // Set up a tm
                  //
                  Domain()
                  {

                  }

                  mockup::domain::Manager domain;
                  mockup::domain::transaction::Manager tm;
               };

            } // <unnamed>
         } // local

         TEST( casual_xatmi_tx, tx_info__expect_0_transaction)
         {
            common::unittest::Trace trace;

            local::Domain domain;

            EXPECT_TRUE( tx_info( nullptr) == 0);

         }

         TEST( casual_xatmi_tx, tx_suspend__no_current_transaction__expect_TX_PROTOCOL_ERROR)
         {
            common::unittest::Trace trace;

            local::Domain domain;

            XID xid;

            ASSERT_TRUE( tx_suspend( &xid) == TX_PROTOCOL_ERROR);
         }

         TEST( casual_xatmi_tx, tx_begin__expect_transaction)
         {
            common::unittest::Trace trace;

            local::Domain domain;

            ASSERT_TRUE( tx_begin() == TX_OK);
            EXPECT_TRUE( tx_info( nullptr) == 1);
            EXPECT_TRUE( tx_commit() == TX_OK);
            EXPECT_TRUE( tx_info( nullptr) == 0);
         }

         TEST( casual_xatmi_tx, two_tx_begin__expect_TX_PROTOCOLL_ERROR)
         {
            common::unittest::Trace trace;

            local::Domain domain;

            ASSERT_TRUE( tx_begin() == TX_OK);
            EXPECT_TRUE( tx_info( nullptr) == 1);
            EXPECT_TRUE( tx_begin() == TX_PROTOCOL_ERROR);
            EXPECT_TRUE( tx_rollback() == TX_OK);
            EXPECT_TRUE( tx_info( nullptr) == 0);
         }

         TEST( casual_xatmi_tx, tx_begin_tx_suspend_tx_resume__tx_rollback__expect_TX_OK)
         {
            common::unittest::Trace trace;

            local::Domain domain;

            XID xid;

            ASSERT_TRUE( tx_begin() == TX_OK);
            ASSERT_TRUE( tx_suspend( &xid) == TX_OK);
            EXPECT_TRUE( tx_info( nullptr) == 0);
            EXPECT_TRUE( tx_resume( &xid) == TX_OK);
            EXPECT_TRUE( tx_rollback() == TX_OK);
         }

         TEST( casual_xatmi_tx, tx_begin__10_tx_suspend_tx_begin___tx_commit_tx_resume__rollback__expect_TX_OK)
         {
            common::unittest::Trace trace;

            local::Domain domain;

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

         TEST( casual_xatmi_tx, tx_begin__10_tx_suspend_tx_begin__in_revers_order__tx_commit_tx_resume__rollback__expect_TX_OK)
         {
            common::unittest::Trace trace;

            local::Domain domain;

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

            for( auto& xid : common::range::make_reverse( xids))
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


         TEST( casual_xatmi_tx, chained_control__tx_begin_tx_commit__expect_new_transaction)
         {
            common::unittest::Trace trace;

            local::Domain domain;

            tx_set_transaction_control( TX_CHAINED);

            ASSERT_TRUE( tx_begin() == TX_OK);
            EXPECT_TRUE( tx_info( nullptr) == 1);
            EXPECT_TRUE( tx_commit() == TX_OK);
            EXPECT_TRUE( tx_info( nullptr) == 1);

            tx_set_transaction_control( TX_UNCHAINED);

            EXPECT_TRUE( tx_commit() == TX_OK);
         }

         TEST( casual_xatmi_tx, chained_control__tx_begin_tx_begin__expect_TX_PROTOCOLL_ERROR)
         {
            common::unittest::Trace trace;

            local::Domain domain;

            tx_set_transaction_control( TX_CHAINED);

            ASSERT_TRUE( tx_begin() == TX_OK);
            EXPECT_TRUE( tx_begin() == TX_PROTOCOL_ERROR);

            tx_set_transaction_control( TX_UNCHAINED);

            EXPECT_TRUE( tx_commit() == TX_OK);
         }

         TEST( casual_xatmi_tx, stacked_control__tx_begin_tx_begin__expect_TX_OK)
         {
            common::unittest::Trace trace;

            local::Domain domain;

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

         TEST( casual_xatmi_tx, stacked_control__tx_begin_tx_begin_tx_suspend__expect_TX_OK)
         {
            common::unittest::Trace trace;

            local::Domain domain;

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

