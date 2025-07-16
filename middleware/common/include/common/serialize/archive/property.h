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
         named =      0b000001,
         order =      0b000010,
         network =    0b000100,
         no_consume = 0b001000, // if the archive is an "adapter" and has no consume semantics
         read =       0b010000, // if the archive is a "reader"
         write =      0b100000, // if the archive is a "writer"
      };

      std::string_view description( Property value) noexcept;

      void casual_enum_as_flag( Property);



      namespace has
      {        
         template< typename T, Property P>
         concept property = requires
         {
            { T::archive_properties() } -> std::same_as< Property>;
            requires ( T::archive_properties() & P) == P;
         };

      } // has

      namespace is
      {

         template< typename T>
         concept dynamic = requires( T a)
         {
            { a.dynamic_properties()} -> std::same_as< Property>;
         };

         template< typename T>
         concept writer = has::property< T, Property::write>;

         template< typename T>
         concept reader = has::property< T, Property::read>;

      } // is

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