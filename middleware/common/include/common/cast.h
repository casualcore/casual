//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_CAST_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_CAST_H_

#include "common/traits.h"

namespace casual
{
   namespace common
   {
      namespace cast
      {


         template< typename E>
         constexpr auto underlying( E value) -> std::enable_if_t< std::is_enum< E>::value, traits::underlying_type_t< E>>
         {
            return static_cast< traits::underlying_type_t< E>>( value);
         }


      } // cast


   } // common



} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_CAST_H_
