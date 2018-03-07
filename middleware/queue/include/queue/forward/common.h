//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_QUEUE_FROWARD_COMMON_H_
#define CASUAL_QUEUE_FROWARD_COMMON_H_

#include "queue/api/message.h"


#include <functional>

namespace casual
{
   namespace queue
   {
      namespace forward
      {
         using dispatch_type = std::function< void( queue::Message&&)>;

         struct Task
         {

            Task( std::string queue, dispatch_type dispatch)
               : queue( std::move( queue)), dispatch( std::move( dispatch)) {}

            std::string queue;
            dispatch_type dispatch;
         };

         struct Dispatch
         {
            Dispatch( std::vector< forward::Task> tasks);

            void execute();

         private:

            std::vector< forward::Task> m_tasks;

         };

      } // forward
   } // queue
} // casual

#endif // COMMON_H_
