//!
//! common.cpp
//!
//! Created on: Nov 30, 2014
//!     Author: Lazan
//!

#include "queue/forward/common.h"
#include "queue/common/queue.h"
#include "queue/api/rm/queue.h"

#include "common/communication/ipc.h"
#include "common/message/dispatch.h"
#include "common/message/handle.h"
#include "common/transaction/context.h"
#include "common/server/handle.h"

#include "queue/rm/switch.h"


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

                     task.dispatch( rm::blocking::dequeue( task.queue));

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
                     return false;
                  }

                  return true;
               }

            } // <unnamed>
         } // local


         Dispatch::Dispatch( std::vector< forward::Task> tasks)
            : Dispatch( std::move( tasks), { { "casual-queue-rm",  &casual_queue_xa_switch_dynamic}}) {}

         Dispatch::Dispatch( std::vector< forward::Task> tasks, const std::vector< common::transaction::Resource>& resources)
           : m_tasks( std::move( tasks))
         {
            if( m_tasks.size() != 1)
            {
               throw common::exception::invalid::Argument{ "only one task is allowed"};
            }

            common::process::instance::connect();
            common::transaction::Context::instance().set( resources);
         }


         void Dispatch::execute()
         {
            while( local::perform( m_tasks.front()))
               ;
         }

      } // forward
   } // queue
} // casual
