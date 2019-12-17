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
               : std::string( std::move( variable)), m_pivot{ variable_pivot( *this)}
            {}

            inline view::String name() const { return { data(), m_pivot};}
            inline view::String value() const 
            {  
               if( m_pivot == range::size( *this)) 
                  return {};

               auto first = data() + m_pivot + 1;
               auto last = data() + size();
               return { first, last};
            }

         private:
            static platform::size::type variable_pivot( const std::string& value)
            {
               auto found = std::find( std::begin( value), std::end( value), '=');
               return std::distance( std::begin( value), found);
            }

            platform::size::type m_pivot{};
         };

         static_assert( traits::is::string::like< Variable>::value, "environment::Variable should be string-like");

      } // environment
   } // common
} // casual