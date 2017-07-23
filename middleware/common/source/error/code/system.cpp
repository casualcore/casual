//!
//! casual
//!

#include "common/error/code/system.h"

#include "common/log.h"

#include <string>

namespace casual
{
   namespace common
   {
      namespace error
      {
         namespace code 
         {

            common::log::Stream& stream( code::system code)
            {
               switch( code)
               {
                  // debug
                  //case casual::ok: return common::log::debug;

                  // rest is errors
                  default: return common::log::category::error;
               }
            }

            namespace last
            {
               namespace system
               {
                  code::system error()
                  {
                     return static_cast< code::system>( errno);
                  }
               } // system
            } // last
            
         } // code 
      } // error
   } // common
} // casual