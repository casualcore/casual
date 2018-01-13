//!
//! casual 
//!

#include "domain/common.h"


namespace casual
{

   namespace domain
   {
      common::log::Stream log{ "casual.domain"};

      namespace trace
      {
         common::log::Stream log{ "casual.domain.trace"};
      } // trace

      namespace verbose
      {
         common::log::Stream log{ "casual.domain.verbose"};
      } // verbose

   } // domain

} // casual
