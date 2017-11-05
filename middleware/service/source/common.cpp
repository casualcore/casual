//!
//! casual 
//!

#include "service/common.h"


namespace casual
{
   namespace service
   {
      common::log::Stream log{ "casual.service"};

      namespace verbose
      {
         common::log::Stream log{ "casual.service.verbose"};
      } // verbose


      namespace trace
      {
         common::log::Stream log{ "casual.service.trace"};
      } // trace

   } // service


} // casual
