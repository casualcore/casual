//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/traits.h"

#include <string>
#include <string_view>
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

            inline std::string_view name() const noexcept{ return { data(), find_pivot( *this)};}
            inline std::string_view value() const noexcept
            {  
               auto pivot = find_pivot( *this) + 1;

               if( pivot > size()) 
                  return {};

               return { data() + pivot, size() - pivot};
            }

         private:
            inline static std::size_t find_pivot( const std::string& value) noexcept
            {
               return std::distance( std::begin( value), std::find( std::begin( value), std::end( value), '='));
            }
         };

         static_assert( traits::is::string::like_v< Variable>, "environment::Variable should be string-like");

         namespace variable::predicate
         {
            inline auto is_name( std::string_view name)
            {
               return [ name]( const Variable& value){ return value.name() == name;};
            }

            inline auto equal_name()
            {
               return []( const Variable& lhs, const Variable& rhs){ return lhs.name() == rhs.name();};
            }

         } // variable::predicate

      } // environment
   } // common
} // casual