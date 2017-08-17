//!
//! casual
//!

#include <common/unittest.h>

#include "common/mockup/domain.h"
#include "common/exception/tx.h"


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
                  mockup::domain::Manager manager;
                  mockup::domain::service::Manager service;
                  mockup::domain::transaction::Manager tm;

               };
            } // <unnamed>
         } // local

         TEST( casual_common_transaction, context_current__expect_null_transaction)
         {
            common::unittest::Trace trace;

            local::Domain domain;

            EXPECT_TRUE( Context::instance().current().trid.null());

         }

         TEST( casual_common_transaction, context_suspend__no_current_transaction__expect_TX_PROTOCOL_ERROR)
         {
            common::unittest::Trace trace;

            local::Domain domain;

            XID xid;

            EXPECT_THROW( Context::instance().suspend( &xid), exception::tx::Protocol);
         }

         TEST( casual_common_transaction, context_begin__expect_transaction)
         {
            common::unittest::Trace trace;

            local::Domain domain;


            ASSERT_TRUE( Context::instance().begin() == code::tx::ok);
            EXPECT_TRUE( ! Context::instance().current().trid.null());
            EXPECT_TRUE( Context::instance().commit() == code::tx::ok);
         }

         TEST( casual_common_transaction, context__two_begin__expect_TX_PROTOCOLL_ERROR)
         {
            common::unittest::Trace trace;

            local::Domain domain;

            ASSERT_TRUE( Context::instance().begin() == code::tx::ok);
            EXPECT_THROW( Context::instance().begin(), exception::tx::Protocol);
            EXPECT_TRUE( Context::instance().rollback() == code::tx::ok);
         }

         TEST( casual_common_transaction, context__begin_suspend_resume__rollback__expect_TX_OK)
         {
            common::unittest::Trace trace;

            local::Domain domain;

            XID xid;

            ASSERT_TRUE( Context::instance().begin() == code::tx::ok);
            ASSERT_NO_THROW( Context::instance().suspend( &xid));
            EXPECT_TRUE( Context::instance().current().trid.null());
            ASSERT_NO_THROW( Context::instance().resume( &xid));
            EXPECT_TRUE( Context::instance().rollback() == code::tx::ok);
         }

         TEST( casual_common_transaction, context__begin__10__suspend_begin_suspend_resume__rollback__expect_TX_OK)
         {
            common::unittest::Trace trace;

            local::Domain domain;

            // global...
            ASSERT_TRUE( Context::instance().begin() == code::tx::ok);

            std::vector< XID> xids( 10);

            // start 10 new ones...
            for( auto& xid : xids)
            {
               ASSERT_NO_THROW( Context::instance().suspend( &xid));
               EXPECT_TRUE( Context::instance().current().trid.null());
               ASSERT_TRUE( Context::instance().begin() == code::tx::ok);
            }

            for( auto& xid : xids)
            {
               EXPECT_TRUE( Context::instance().commit() == code::tx::ok);
               ASSERT_NO_THROW( Context::instance().resume( &xid)) << "xid: " << xid;
               EXPECT_TRUE( Context::instance().current().trid == xid);
            }

            EXPECT_TRUE( Context::instance().rollback() == code::tx::ok);
         }


      } // transaction
   } // common
} // casual
