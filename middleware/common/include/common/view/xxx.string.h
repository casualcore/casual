//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

// TODO: remove this when we switch to c++17


#include "common/range.h"

#include <string>

namespace casual
{
   namespace common
   {
      namespace view
      {
         namespace string
         {
            namespace value
            {
               using type = platform::character::type;
            } // value
         }

         struct String : Range< const string::value::type*>
         {
            using base_type = Range< const string::value::type*>;

            // some versions of g++ doesn't like constexpr for ctors and stuff, we don't really need it so...
            // constexpr String() noexcept = default;
            // constexpr String(const String&) noexcept = default;
            // constexpr String& operator = ( const String&) noexcept = default;
            
            String() noexcept = default;
            String( const String&) noexcept = default;
            String& operator = ( const String&) noexcept = default;

            constexpr String( const string::value::type* string, platform::size::type count)
               : base_type( string, count) {}

            inline String( const platform::character::type* string)
               : String( string, std::char_traits< string::value::type>::length( string)) {}
            
            inline String( iterator first, iterator last)
               : base_type( first, last) {}

            template< typename T, typename = std::enable_if_t< std::is_convertible< decltype( &(*std::declval< T>())), iterator>::value>>
            constexpr String( Range< T> range) : String( range.data(), range.size()) {};

            inline String( const std::string& other) noexcept : String( other.data(), other.size()) {}
            
            //! @return concrete std::string
            inline std::string value() const { return { begin(), end()};}

            inline friend bool operator == ( const String& lhs, const String& rhs) 
            {
               return std::equal( std::begin( lhs), std::end( lhs), std::begin( rhs), std::end( rhs));
            };

            inline friend std::ostream& operator << ( std::ostream& out, const String& value)
            {
               return out.write( value.data(), value.size());
            }
         };

      } // view
   } // common
} // casual