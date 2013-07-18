//!
//! context.h
//!
//! Created on: Jul 14, 2013
//!     Author: Lazan
//!

#ifndef CONTEXT_H_
#define CONTEXT_H_

#include <tx.h>

#include "common/ipc.h"

namespace casual
{
   namespace common
   {
      namespace transaction
      {
         struct State
         {
            ipc::send::Queue::queue_key_type transactionManagerQueue = 0;
         };

         class Context
         {
         public:

            static Context& instance();

            int begin();
            int close();
            int commit();
            int rollback();
            int open();
            int setCommitReturn( COMMIT_RETURN value);
            int setTransactionControl(TRANSACTION_CONTROL control);
            int setTransactionTimeout(TRANSACTION_TIMEOUT timeout);
            int info( TXINFO& info);


            State& state();

         private:

            Context();

            State m_state;

         };

      } // transaction
   } // common
} // casual



#endif /* CONTEXT_H_ */
