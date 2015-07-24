//!
//! test_transaction.cpp
//!
//! Created on: Mar 7, 2015
//!     Author: Lazan
//!

#include <gtest/gtest.h>

#include "common/mockup/domain.h"


#include "common/transaction/context.h"


#include "common/trace.h"
#include "common/internal/log.h"

namespace casual
{
   namespace common
   {
      namespace transaction
      {

         TEST( casual_common_transaction, context_current__expect_null_transaction)
         {
            mockup::domain::Domain domain;

            EXPECT_TRUE( Context::instance().current().trid.null());

         }

         TEST( casual_common_transaction, context_suspend__no_current_transaction__expect_TX_PROTOCOL_ERROR)
         {
            mockup::domain::Domain domain;

            XID xid;

            EXPECT_THROW( Context::instance().suspend( &xid), exception::tx::Protocoll);
         }

         TEST( casual_common_transaction, context_begin__expect_transaction)
         {
            Trace trace{ "casual_common_transaction.context_begin__expect_transaction", log::internal::debug};

            mockup::domain::Domain domain;


            ASSERT_TRUE( Context::instance().begin() == TX_OK);
            EXPECT_TRUE( ! Context::instance().current().trid.null());
            EXPECT_TRUE( Context::instance().commit() == TX_OK);
         }

         TEST( casual_common_transaction, context__two_begin__expect_TX_PROTOCOLL_ERROR)
         {
            mockup::domain::Domain domain;

            ASSERT_TRUE( Context::instance().begin() == TX_OK);
            EXPECT_THROW( Context::instance().begin(), exception::tx::Protocoll);
            EXPECT_TRUE( Context::instance().rollback() == TX_OK);
         }

         TEST( casual_common_transaction, context__begin_suspend_resume__rollback__expect_TX_OK)
         {
            mockup::domain::Domain domain;

            XID xid;

            ASSERT_TRUE( Context::instance().begin() == TX_OK);
            ASSERT_NO_THROW( Context::instance().suspend( &xid));
            EXPECT_TRUE( Context::instance().current().trid.null());
            ASSERT_NO_THROW( Context::instance().resume( &xid));
            EXPECT_TRUE( Context::instance().rollback() == TX_OK);
         }

         TEST( casual_common_transaction, context__begin__10__suspend_begin_suspend_resume__rollback__expect_TX_OK)
         {
            mockup::domain::Domain domain;

            // global...
            ASSERT_TRUE( Context::instance().begin() == TX_OK);

            std::vector< XID> xids( 10);

            // start 10 new ones...
            for( auto& xid : xids)
            {
               ASSERT_NO_THROW( Context::instance().suspend( &xid));
               EXPECT_TRUE( Context::instance().current().trid.null());
               ASSERT_TRUE( Context::instance().begin() == TX_OK);
            }

            for( auto& xid : xids)
            {
               EXPECT_TRUE( Context::instance().commit() == TX_OK);
               ASSERT_NO_THROW( Context::instance().resume( &xid)) << "xid: " << xid;
               EXPECT_TRUE( Context::instance().current().trid == xid);
            }

            EXPECT_TRUE( Context::instance().rollback() == TX_OK);
         }


      } // transaction
   } // common
} // casual
