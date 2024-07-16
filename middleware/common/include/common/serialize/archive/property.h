//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/concepts/serialize.h"
#include "common/flag/enum.h"

#include <string_view>

namespace casual
{
   namespace common::serialize::archive
   {
      enum struct Property
      {
         named =      0b0001,
         order =      0b0010,
         network =    0b0100,
         no_consume = 0b1000, // if the archive is an "adapter" and has no consume semantics
      };

      std::string_view description( Property value) noexcept;

      void casual_enum_as_flag( Property);

      namespace is
      {
         template< typename T> 
         concept not_dynamic = requires
         {
            { T::archive_properties() } -> std::same_as< Property>;
         };

         template< typename T>
         concept dynamic = ! not_dynamic< T> && requires( T a)
         {
            { a.archive_properties()} -> std::same_as< Property>;
         };
      } // is

      namespace has
      {        
         template< typename T, Property P>
         concept property = requires
         {
            { T::archive_properties() } -> std::same_as< Property>;
            requires ( T::archive_properties() & P) == P;
         };

      } // has

      namespace need
      {
         template< typename T>
         concept named = is::dynamic< T> || has::property< T, Property::named>;

         template< typename T>
         concept order = ! is::dynamic< T> && has::property< T, Property::order>;

      } // need

      namespace is::network
      {
         template< typename T>
         concept normalizing = has::property< T, Property::network>;

      } // is::network



   } // common::serialize::archive
} // casual