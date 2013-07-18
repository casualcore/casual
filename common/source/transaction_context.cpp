//!
//! context.cpp
//!
//! Created on: Jul 14, 2013
//!     Author: Lazan
//!

#include "common/transaction_context.h"


namespace casual
{
   namespace common
   {

      namespace transaction
      {
         Context& Context::instance()
         {
            static Context singleton;
            return singleton;
         }

         Context::Context()
         {

         }

         int Context::begin()
         {
            return TX_OK;
         }

         int Context::close()
         {
            return TX_OK;
         }

         int Context::commit()
         {
            return TX_OK;
         }

         int Context::rollback()
         {
            return TX_OK;
         }

         int Context::open()
         {
            return TX_OK;
         }

         int Context::setCommitReturn( COMMIT_RETURN value)
         {
            return TX_OK;
         }

         int Context::setTransactionControl(TRANSACTION_CONTROL control)
         {
            return TX_OK;
         }

         int Context::setTransactionTimeout(TRANSACTION_TIMEOUT timeout)
         {
            return TX_OK;
         }

         int Context::info( TXINFO& info)
         {
            return TX_OK;
         }

         State& Context::state()
         {
            return m_state;
         }
      } // transaction
   } // common
} //casual

