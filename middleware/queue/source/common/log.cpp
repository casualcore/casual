//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "queue/common/log.h"



namespace casual
{
   namespace queue
   {
      common::log::Stream log{ "casual.queue"};

      namespace verbose
      {
         common::log::Stream log{ "casual.queue.verbose"};  
      } // verbose

      namespace trace
      {
         common::log::Stream log{ "casual.queue.trace"};  
      } // trace

      namespace event
      {
         common::log::Stream log{ "casual.event.queue"};
      } // event
      
   } // queue
} // casual
