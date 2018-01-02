//!
//! casual 
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
      } // verbose

   } // queue
} // casual
