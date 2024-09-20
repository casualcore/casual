//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

namespace casual
{
   namespace common::algorithm::compare
   {      
      //! @returns true if `value` is equal to any other `values`
      template< typename V, typename... Vs>
      constexpr bool any( V&& value, Vs&&... values)
      {
         auto equal = []( auto&& l, auto& r){ return l == r;}; // to mitigate g++ 9.3.1 bug
         return ( equal( value, values) || ... ); 
      }


   } // common::algorithm::compare
} // casual