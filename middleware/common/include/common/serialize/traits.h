//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#pragma once


#include "common/serialize/archive/type.h"

#include "common/traits.h"
#include "common/view/binary.h"
#include "common/string/utf8.h"
#include "casual/platform.h"

#include <string>

namespace casual
{
   namespace common::serialize::traits
   {
      using namespace common::traits;

      namespace archive
      {

         template< typename A>
         inline constexpr auto type_v = std::decay_t< A>::archive_type();


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
            inline constexpr auto convert_v = detail::convert< archive::type_v< A>>::value;

         } // dynamic
         
      } // archive

      namespace need
      {
         template< typename A>
         inline constexpr bool named_v = archive::type_v< A> != serialize::archive::Type::static_order_type;

      } // need

      namespace has
      {
         namespace detail
         {
            template< typename T, typename A>
            using serialize = decltype( std::declval< std::remove_reference_t< T>&>().serialize( std::declval< traits::remove_cvref_t< A>&>()));
         } // detail

         template< typename T, typename A>
         inline constexpr bool serialize_v = detect::is_detected_v< detail::serialize, T, A>;

         namespace forward
         {
            namespace detail
            {
               template< typename T, typename A>
               using serialize = decltype( std::declval< std::remove_reference_t< T>&>().serialize( std::declval< traits::remove_cvref_t< A>&>(), ""));
            } // detail

            template< typename T, typename A>
            inline constexpr bool serialize_v = detect::is_detected_v< detail::serialize, T, A>;
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
         inline constexpr bool serialize_v = detect::is_detected_v< detail::serialize, T, A>;
         
      } // can

      namespace is
      {
         //! all "pods" that can be serialized directly
         template< typename T>
         inline constexpr bool pod_v = ( std::is_pod_v< T> && ! std::is_class_v< T> && ! std::is_enum_v< T>) // normal pods
            || common::traits::is::any_v< T, std::string, platform::binary::type>;

         namespace archive
         {
            namespace write
            {
               //! predicate which types a write-archive can write 'natively'
               template< typename T> 
               inline constexpr bool type_v = common::traits::is::any_v< common::traits::remove_cvref_t< T>, 
                        bool, char, short, int, long, unsigned long, long long, float, double,
                        view::immutable::Binary, view::Binary, string::immutable::utf8, string::utf8>
                  || common::traits::has::any::base_v< common::traits::remove_cvref_t< T>, std::string, platform::binary::type>;
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
            inline constexpr bool value_v = detect::is_detected_v< detail::value, V>;
            
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

            template< typename A>
            inline constexpr bool normalizing_v = normalizing< A>::value;
         } // network

         namespace message
         {
            namespace detail
            {
               template< typename T>
               using like = decltype( std::declval< T&>().type(), std::declval< T&>().correlation, std::declval< T&>().execution);

            } // detail
            template< typename T>
            inline constexpr bool like_v = detect::is_detected_v< detail::like, T>;
         } // message
      
      } // is

   } // common::serialize::traits
} // casual

