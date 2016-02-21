//!
//! casual 
//!

#include "gateway/environment.h"


namespace casual
{
   namespace gateway
   {
      namespace environment
      {
         const common::Uuid& identification()
         {
            static const common::Uuid id{ "b9624e2f85404480913b06e8d503fce5"};
            return id;
         }

      } // environment
   } // gateway


} // casual
