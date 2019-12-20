//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "common/view/string.h"
#include "common/traits.h"

#include <string>
#include <algorithm>

namespace casual
{
   namespace common
   {
      namespace environment
      {
         struct Variable : std::string
         {
            Variable() = default;
            inline Variable( std::string variable) 
               : std::string( std::move( variable))
            {}



            inline view::String name() const { return { data(), data() + find_pivot( *this)};}
            inline view::String value() const 
            {  
               auto pivot = find_pivot( *this);

               if( pivot == size()) 
                  return {};

               return { data() + pivot + 1,  data() + size()};
            }

         private:
            static std::size_t find_pivot( const std::string& value)
            {
               return std::distance( std::begin( value), std::find( std::begin( value), std::end( value), '='));
            }
         };

         static_assert( traits::is::string::like< Variable>::value, "environment::Variable should be string-like");

      } // environment
   } // common
} // casual