//!
//! casual
//!

#include "queue/forward/common.h"
#include "queue/common/queue.h"
#include "queue/api/queue.h"

#include "common/communication/ipc.h"
#include "common/message/dispatch.h"
#include "common/message/handle.h"
#include "common/transaction/context.h"



#include "tx.h"

namespace casual
{
   namespace queue
   {
      namespace forward
      {
         namespace local
         {
            namespace
            {
               bool perform( const forward::Task& task)
               {
                  try
                  {
                     if( tx_begin() != TX_OK)
                     {
                        return false;
                     }

                     //
                     // Rollback unless we commit
                     //
                     auto rollback = common::scope::execute( [](){
                           tx_rollback();
                     });

                     task.dispatch( blocking::dequeue( task.queue));

                     //
                     // Check what we should do with the transaction
                     //
                     {
                        TXINFO txinfo;
                        tx_info( &txinfo);

                        if( txinfo.transaction_state == TX_ACTIVE)
                        {
                           tx_commit();
                           rollback.release();
                        }
                     }
                  }
                  catch( const common::exception::Shutdown&)
                  {
                     return false;
                  }
                  catch( ...)
                  {
                     common::error::handler();
                     //return false;
                  }

                  return true;
               }

            } // <unnamed>
         } // local


         Dispatch::Dispatch( std::vector< forward::Task> tasks)
           : m_tasks( std::move( tasks))
         {
            if( m_tasks.size() != 1)
            {
               throw common::exception::invalid::Argument{ "only one task is allowed"};
            }

            common::process::instance::connect();
         }


         void Dispatch::execute()
         {
            while( local::perform( m_tasks.front()))
               ;
         }

      } // forward
   } // queue
} // casual
