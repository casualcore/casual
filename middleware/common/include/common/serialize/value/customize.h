//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/traits.h"

namespace casual
{
   namespace common
   {
      namespace serialize
      {
         namespace customize
         {
            //! customization point for serialization
            template< typename T, typename A, typename Enable = void> 
            struct Value;

            namespace traits
            {
               template< typename T, typename A>
               using value_t = customize::Value< common::traits::remove_cvref_t< T>, common::traits::remove_cvref_t< A>>;
            } // traits

            namespace composit
            {
               //! customization point for serialization
               template< typename T, typename A, typename Enable = void> 
               struct Value;

               namespace traits
               {
                  template< typename T, typename A>
                  using value_t = composit::Value< common::traits::remove_cvref_t< T>, common::traits::remove_cvref_t< A>>;
               } // traits
               
            } // composit

            namespace forward
            {
               //! customization point for forward
               template< typename T, typename A, typename Enable = void> 
               struct Value;

               namespace traits
               {
                  template< typename T, typename A>
                  using value_t = composit::Value< common::traits::remove_cvref_t< T>, common::traits::remove_cvref_t< A>>;
               } // traits
               
            } // composit
            
         } // customize
         
      } // serialize
   } // common
} // casual