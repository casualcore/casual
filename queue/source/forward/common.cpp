//!
//! common.cpp
//!
//! Created on: Nov 30, 2014
//!     Author: Lazan
//!

#include "queue/forward/common.h"
#include "queue/common/queue.h"

#include "common/queue.h"

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
               std::vector< Dispatch::Task> lookup( const std::vector< forward::Task>& tasks)
               {
                  std::vector< Dispatch::Task> result;

                  for( auto& task : tasks)
                  {
                     auto reply = queue::Lookup{ task.queue}();

                     if( reply.queue == 0)
                     {
                        throw common::exception::invalid::Argument{ "failed to lookup queue: " + task.queue};
                     }

                     Dispatch::Task dispatch;

                     dispatch.process = reply.process;
                     dispatch.queue.id = reply.queue;
                     dispatch.queue.name = task.queue;

                     result.push_back( std::move( dispatch));
                  }

                  return result;
               }

            } // <unnamed>
         } // local


         Dispatch::Dispatch( const std::vector< forward::Task>& tasks)
            : m_tasks( local::lookup( tasks))
         {

         }

         void Dispatch::execute()
         {

         }

      } // forward
   } // queue
} // casual
