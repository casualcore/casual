//!
//! casual 
//!

#include "xatmi.h"

namespace casual
{

   namespace example
   {
      namespace server
      {

         extern "C"
         {
            void casual_example_echo( TPSVCINFO *info)
            {
               tpreturn( TPSUCCESS, 0, info->data, info->len, 0);
            }

         }
      } // server
   } // example
} // casual
