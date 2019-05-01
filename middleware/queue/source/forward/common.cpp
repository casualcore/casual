//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "queue/forward/common.h"
#include "queue/common/log.h"
#include "queue/common/queue.h"
#include "queue/api/queue.h"

#include "common/communication/ipc.h"
#include "common/message/dispatch.h"
#include "common/message/handle.h"
#include "common/exception/casual.h"
#include "common/transaction/context.h"
#include "common/execute.h"

#include "common/communication/instance.h"

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
                     Trace trace{ "queue::forward::local::perform"};

                     common::transaction::context().begin();

                     // Rollback unless we commit
                     auto rollback = common::execute::scope( [](){
                        common::transaction::context().rollback();
                     });

                     task.dispatch( blocking::available::dequeue( task.queue));

                     // Check what we should do with the transaction
                     {
                        TXINFO txinfo;
                        common::transaction::context().info( &txinfo);

                        if( txinfo.transaction_state == TX_ACTIVE)
                        {
                           common::transaction::context().commit();
                           rollback.release();
                        }
                     }
                  }
                  catch( const common::exception::casual::Shutdown&)
                  {
                     return false;
                  }
                  catch( ...)
                  {
                     common::exception::handle();
                     return false;
                  }

                  return true;
               }

            } // <unnamed>
         } // local


         Dispatch::Dispatch( std::vector< forward::Task> tasks)
           : m_tasks( std::move( tasks))
         {
            Trace trace{ "queue::forward::Dispatch::Dispatch"};

            if( m_tasks.size() != 1)
               throw common::exception::system::invalid::Argument{ "only one task is allowed"};

            // wait for queue-manager to be up and running
            {
               auto manager = common::communication::instance::fetch::handle( 
                  common::communication::instance::identity::queue::manager,
                  common::communication::instance::fetch::Directive::wait);

               common::log::line( verbose::log, "queue-manager is running: ", manager);
            }

         
            // connect to domain
            common::communication::instance::connect();
         }


         void Dispatch::execute()
         {
            while( local::perform( m_tasks.front()))
               ;
         }

      } // forward
   } // queue
} // casual
