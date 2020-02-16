
#include "common/result.h"

#include "common/code/system.h"
#include "common/exception/system.h"

namespace casual
{
   namespace common
   {
      namespace posix
      {
  
         int result( int result)
         {
            if( result == -1)
               exception::system::throw_from_code( common::code::last::system::error());

            return result;
         }

         namespace error
         {
            optional_error optional( int result) noexcept
            {
               if( result == -1)
                  return optional_error{ common::code::last::system::error()};

               return {};
            }
         } // error


      } // posix
   } // common
} // casual