//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#pragma once


#include "common/serialize/archive/type.h"

#include "common/traits.h"
#include "common/view/binary.h"
#include "casual/platform.h"

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


            namespace archive
            {

               template< typename A>
               struct type 
               {
                  constexpr static auto value = std::decay_t< A>::archive_type();
               };

               namespace dynamic
               {
                  namespace detail
                  {
                     template< serialize::archive::Type>
                     struct convert;

                     template<>
                     struct convert< serialize::archive::Type::static_need_named>
                     {
                        constexpr static auto value = serialize::archive::dynamic::Type::named;
                     };

                     template<>
                     struct convert< serialize::archive::Type::static_order_type>
                     {
                        constexpr static auto value = serialize::archive::dynamic::Type::order_type;
                     };
                  } // detail

                  template< typename A>
                  struct convert 
                  {
                     constexpr static auto value = detail::convert< archive::type< A>::value>::value;
                  };

               } // dynamic
               
            } // archive

            namespace need
            {
               namespace detail
               {
                  template< typename A>
                  using named = typename A::need_named;
               } // detail

               template< typename A>
               using named = bool_constant< archive::type< A>::value != serialize::archive::Type::static_order_type>;

            } // need

            namespace has
            {
               namespace detail
               {
                  template< typename T, typename A>
                  using serialize = decltype( std::declval< std::remove_reference_t< T>&>().serialize( std::declval< traits::remove_cvref_t< A>&>()));
               } // detail

               template< typename T, typename A>
               using serialize = detect::is_detected< detail::serialize, T, A>;

               namespace forward
               {
                  namespace detail
                  {
                     template< typename T, typename A>
                     using serialize = decltype( std::declval< std::remove_reference_t< T>&>().serialize( std::declval< traits::remove_cvref_t< A>&>(), ""));
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

               namespace archive
               {
                  namespace write
                  {
                     //! predicate which types a write-archive can write 'natively'
                     template< typename T> 
                     using type = traits::bool_constant< 
                        traits::is_any< common::traits::remove_cvref_t< T>, 
                           bool, char, short, int, long, unsigned long, long long, float, double,
                           view::immutable::Binary, view::Binary>::value
                        || common::traits::has::any::base< common::traits::remove_cvref_t< T>, std::string, platform::binary::type>::value
                     >;
                  } // read

                  namespace detail
                  {
                     template< typename A>
                     constexpr auto input( A& archive, priority::tag< 0>) { return false;}

                     template< typename A>
                     constexpr auto input( A& archive, priority::tag< 1>) -> decltype( void( archive >> std::declval< long&>()), bool())
                     {
                        return true;
                     }
                  } // detail

                  template< typename A>
                  constexpr auto input( A& archive) -> decltype( detail::input( archive, priority::tag< 1>{}))
                  {
                     return detail::input( archive, priority::tag< 1>{});
                  }
               } // archive

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

               namespace message
               {
                  namespace detail
                  {
                     template< typename T>
                     using like = decltype( std::declval< T&>().type(), std::declval< T&>().correlation, std::declval< T&>().execution);

                  } // detail
                  template< typename T>
                  using like = detect::is_detected< detail::like, T>;
               } // message
            
            } // is
         } // traits
      } // serialize
   } // common
} // casual

