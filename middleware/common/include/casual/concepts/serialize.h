//!
//! Copyright (c) 2023, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/concepts.h"

#include "common/flag/enum.h"
#include "common/view/binary.h"


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
            concept type = concepts::decayed::any_of< T, bool, char, short, long, long long, float, double, 
               common::view::immutable::Binary, common::view::Binary>
               || concepts::derived_from< T, std::string, std::u8string, platform::binary::type>;
         } // native

      } // archive

      namespace need
      {
         template< typename T>
         inline constexpr bool named = T::archive_type() != decltype( T::archive_type())::static_order_type;

      } // need

      namespace named
      {
         template< typename T>
         concept value = requires
         {
            typename T::serialize_named_value_type;
         };
      } // named

      //namespace network
      //{
      //   template< typename T>
      //   concept normalizing = requires
      //   {
      //      typename T::is_network_normalizing;  
      //   };
      //} // network
      
   } // concepts::serialize
} // casual
