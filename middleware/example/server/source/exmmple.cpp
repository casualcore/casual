//!
//! casual 
//!

#include "xatmi.h"

#include "common/process.h"

#include <chrono>

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
               //common::process::sleep( std::chrono::milliseconds{ 50});
               tpreturn( TPSUCCESS, 0, info->data, info->len, 0);
            }

         }
      } // server
   } // example
} // casual
