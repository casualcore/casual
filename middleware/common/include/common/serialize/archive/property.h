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
         named = 1,
         order = 2,
      };

      std::string_view description( Property value) noexcept;

      void casual_enum_as_flag( Property);

      namespace is
      {
         template< typename T>
         concept dynamic = requires
         {
            typename T::dynamic_archive;
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

      namespace network
      {
         namespace detail
         {
            template< typename A>
            concept normalizing = requires 
            {  
               typename A::is_network_normalizing;
            };
         } // detail

         //! default instance -> all types that has a typedef of `is_network_normalizing`
         template< typename A>
         struct normalizing : std::bool_constant< detail::normalizing< A>>{};

         template< typename A>
         inline constexpr bool normalizing_v = normalizing< A>::value;
      } // network



   } // common::serialize::archive
} // casual