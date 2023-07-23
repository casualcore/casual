//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/traits.h"

namespace casual
{
   
   namespace common::serialize::customize
   {
      //! customization point for serialization
      template< typename T, typename A> 
      struct Value;

      namespace traits
      {
         template< typename T, typename A>
         using value_t = customize::Value< std::remove_cvref_t< T>, std::remove_cvref_t< A>>;
      } // traits

      namespace composite
      {
         //! customization point for serialization
         template< typename T, typename A> 
         struct Value;

         namespace traits
         {
            template< typename T, typename A>
            using value_t = composite::Value< std::remove_cvref_t< T>, std::remove_cvref_t< A>>;
         } // traits
         
      } // composite

   } // common::serialize::customize
   
} // casual