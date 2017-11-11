//!
//! casual 
//!

#include "event/common.h"


namespace casual
{
   namespace event
   {
      common::log::Stream log{ "casual.event"};

      namespace verbose
      {
         common::log::Stream log{ "casual.event.verbose"};
      } // verbose

      namespace trace
      {
         common::log::Stream log{ "casual.trace"};
      } // verbose

   } // traffic
} // casual
