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
         int begin()
         {
            return TX_OK;
         }

         int close()
         {
            return TX_OK;
         }

         int commit()
         {
            return TX_OK;
         }

         int rollback()
         {
            return TX_OK;
         }

         int open()
         {
            return TX_OK;
         }

         int setCommitReturn( COMMIT_RETURN value)
         {
            return TX_OK;
         }

         int setTransactionControl(TRANSACTION_CONTROL control)
         {
            return TX_OK;
         }

         int setTransactionTimeout(TRANSACTION_TIMEOUT timeout)
         {
            return TX_OK;
         }

         int info( TXINFO& info)
         {
            return TX_OK;
         }
      } // transaction
   } // common
} //casual

