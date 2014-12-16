//!
//! common.h
//!
//! Created on: Nov 30, 2014
//!     Author: Lazan
//!

#ifndef CASUAL_QUEUE_FROWARD_COMMON_H_
#define CASUAL_QUEUE_FROWARD_COMMON_H_

#include "common/message/queue.h"

#include "common/transaction/resource.h"


#include <functional>

namespace casual
{
   namespace queue
   {
      namespace forward
      {
         using dispatch_type = std::function< void( common::message::queue::dequeue::Reply::Message&&)>;

         struct Task
         {

            Task( std::string queue, dispatch_type dispatch)
               : queue( std::move( queue)), dispatch( std::move( dispatch)) {}

            std::string queue;
            dispatch_type dispatch;
         };

         struct Dispatch
         {
            Dispatch( const std::vector< forward::Task>& tasks);
            Dispatch( const std::vector< forward::Task>& tasks, const std::vector< common::transaction::Resource>& resources);

            void execute();

            struct Task
            {
               struct queue_t
               {
                  std::size_t id;
                  std::string name;
               } queue;

               common::process::Handle process;
               dispatch_type dispatch;
            };

         private:

            std::vector< Task> m_tasks;

         };

      } // forward
   } // queue
} // casual

#endif // COMMON_H_
