//!
//! casual
//!

#ifndef CASUAL_COMMON_ERROR_CODE_SYSTEM_H_
#define CASUAL_COMMON_ERROR_CODE_SYSTEM_H_

#include "common/log/stream.h"

#include <system_error>

namespace casual
{
   namespace common
   {
      namespace error
      {
         namespace code 
         {

            using system = std::errc;
            

            common::log::Stream& stream( system code);
            
            namespace last
            {
               namespace system
               {
                  //!
                  //! returns the last error code from errno
                  //!
                  code::system error();
               } // system

            } // last
         
         } // code 
      } // error

   } // common
} // casual

/*
namespace casual
{
   namespace common
   {
      namespace error
      {
         namespace code 
         {
            inline std::ostream& operator << ( std::ostream& out, code::system value) 
            { 
               const auto condition = std::make_error_condition( value);
               return out << condition.category().name() << ':' << condition.value() << " - " << condition.message();
            }
         } // code 
      } // error
   } // common
} // casual
*/

namespace std
{
   inline std::ostream& operator << ( std::ostream& out, std::errc value) 
   { 
      const auto code = std::make_error_code( value);
      return out << code  << " - " << code.message();
   }
} // std

#endif