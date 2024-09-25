//!
//! Copyright (c) 2023, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/concepts.h"

#include "common/flag/enum.h"
#include "common/binary/span.h"


namespace casual
{
   namespace concepts::serialize
   {
      namespace has
      {
         template< typename T, typename A>
         concept serialize = requires( T a, A& archive)
         {
            a.serialize( archive);
         };
         
      } // has

      namespace archive
      {
         namespace native
         {
            //! predicate which types an archive can read/write 'natively'
            template< typename T> 
            concept read = concepts::decayed::any_of< T, bool, char, short, int, long, long long, float, double, 
               common::binary::span::Fixed< std::byte>>
               || concepts::derived_from< T, std::string, std::u8string, platform::binary::type>;

            template< typename T> 
            concept write = concepts::decayed::any_of< T, bool, char, short, int, long, long long, float, double, 
               std::string_view, std::u8string_view, std::span< const std::byte>, common::binary::span::Fixed< const std::byte>>;

            template< typename T> 
            concept read_write = read< T> && write< T>;
         } // native

      } // archive

      namespace named
      {
         template< typename T>
         concept value = requires
         {
            typename T::serialize_named_value_type;
         };
      } // named

      
   } // concepts::serialize
} // casual
