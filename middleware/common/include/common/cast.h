//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


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


