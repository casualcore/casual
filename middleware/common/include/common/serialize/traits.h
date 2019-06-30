//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#pragma once


#include "common/traits.h"
#include "common/platform.h"

#include <string>

namespace casual
{
   namespace common
   {
      namespace serialize
      {
         namespace traits
         {
            using namespace common::traits;

            namespace need
            {
               namespace detail
               {
                  template< typename A>
                  using named = typename A::need_named;
               } // detail
               
               template< typename A>
               using named = detect::is_detected< detail::named, A>;

            } // need


            namespace has
            {
               namespace detail
               {
                  template< typename T, typename A>
                  using serialize = decltype( std::declval< traits::remove_cvref_t< T>&>().serialize( std::declval< traits::remove_cvref_t< A>&>()));
               } // detail

               template< typename T, typename A>
               using serialize = detect::is_detected< detail::serialize, T, A>;

               namespace forward
               {
                  namespace detail
                  {
                     template< typename T, typename A>
                     using serialize = decltype( std::declval< traits::remove_cvref_t< T>&>().serialize( std::declval< traits::remove_cvref_t< A>&>(), ""));
                  } // detail

                  template< typename T, typename A>
                  using serialize = detect::is_detected< detail::serialize, T, A>;
               } // forward
            } // has

            namespace can
            {
               namespace detail
               {
                  template< typename T, typename A>
                  using serialize = decltype( std::declval< traits::remove_cvref_t< A>&>() << std::declval< traits::remove_cvref_t< T>&>());
               } // detail

               template< typename T, typename A>
               using serialize = detect::is_detected< detail::serialize, T, A>;
               
            } // can

            namespace is
            {
               //! all "pods" that can be serialized directly
               template< typename T>
               using pod = traits::bool_constant< 
                  ( std::is_pod< T>::value && ! std::is_class< T>::value && ! std::is_enum< T>::value ) // normal pods
                  || std::is_same< T, std::string>::value 
                  || std::is_same< T, platform::binary::type>::value 
               >;

               namespace named
               {
                  namespace detail
                  {
                     template< typename V>
                     using value = typename V::serialize_named_value_type;
                  } // detail

                  template< typename V>
                  using value = detect::is_detected< detail::value, V>;
                  
               } // named               

               namespace network
               {
                  namespace detail
                  {
                     template< typename A>
                     using normalizing = typename A::is_network_normalizing;
                  } // detail
                  template< typename A>
                  struct normalizing : bool_constant< detect::is_detected< detail::normalizing, A>::value>{};
               } // network
            } // is
         } // traits
      } // serialize
   } // common
} // casual

