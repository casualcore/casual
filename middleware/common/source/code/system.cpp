//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/code/system.h"

#include "common/log.h"

#include <string>

namespace casual
{
   namespace common
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
   } // common
} // casual
