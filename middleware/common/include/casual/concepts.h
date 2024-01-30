//!
//! Copyright (c) 2023, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include <ranges>

namespace casual
{
   namespace concepts
   {
      //! true if `T` is the same as any of the types in pack `Ts`
      template<typename T, typename ... Ts>
      concept any_of = ( std::same_as< T, Ts> || ...);

      //! true if `T` is the same as all of the types in pack `Ts`
      template< typename T, typename... Ts>
      concept same_as = ( std::same_as< T, Ts> && ...);

      namespace decayed
      {
         //! true if `std::remove_cvref_t< T>` is the same as any of the types in pack `Ts`
         template<typename T, typename ... Ts>
         concept any_of = ( std::same_as< std::remove_cvref_t< T>, Ts> || ...);

         //! true if ` std::remove_cvref_t< T>` is the same as all of the types in pack `Ts`
         template< typename T, typename... Ts>
         concept same_as = ( std::same_as< std::remove_cvref_t< T>, Ts> && ...);
      } // decayed

      template< typename T>
      concept range = std::ranges::range< T>;

      template< typename R, typename V>
      concept range_with_value = concepts::range< R> && requires( R& a)
      {
         { *std::begin( a)} -> concepts::decayed::same_as< V>;
      };

      template< typename T>
      concept arithmetic = std::integral< T> || std::floating_point< T>;

      template< typename T>
      concept movable = requires { std::is_move_constructible_v< T> && std::is_move_assignable_v< T>; };

      namespace nothrow
      {
         template< typename T>
         concept movable = requires { std::is_nothrow_move_constructible_v< T> && std::is_nothrow_move_assignable_v< T>; };         
      } // nothrow

      template< typename T>
      concept copyable = std::is_copy_constructible_v< T> && std::is_copy_assignable_v< T>;

      namespace tuple
      {
         template< typename T>
         concept size = requires { std::tuple_size< T>::value; };

         template< typename T>
         concept like = tuple::size< T> && ! concepts::range< T>; 
      } // tuple

      namespace compare
      {
         template< typename T> 
         concept less = requires( const std::remove_reference_t< T>& lhs, const std::remove_reference_t< T>& rhs) 
         {
            { lhs < rhs} -> std::convertible_to< bool>;
         };
         
      } // compare

      namespace optional
      {
         template< typename T>
         concept like = requires( T a) {
            { a.has_value()} -> std::convertible_to< bool>;
            //{ a.value() == *a};
         };
         
      } // optional

      namespace container
      {
         template< typename T>
         concept insert = concepts::range< T> && 
         requires( T a) { a.insert( std::end( a), std::begin( a), std::end( a));};

         namespace value
         {
            template<typename T>
            concept insert = concepts::range< T> && 
            requires( T a) { a.insert( *std::begin( a));};
         } // value

         template< typename T>
         concept resize = concepts::range< T> && 
         requires( T a) { a.resize( a.size()); };

         template< typename T>
         concept reserve = concepts::range< T> && 
         requires( T a) { a.reserve( a.size()); };

         template< typename T, typename Iter>
         concept erase = concepts::range< T> &&
         requires( T container, Iter iterator) 
         { 
            container.erase( iterator);
         };

         template< typename T, typename R>
         concept erase_range = concepts::range< T> && concepts::range< R> &&
         requires( T container, R range) 
         { 
            container.erase( std::begin( range), std::end( range));
         };

         template< typename T>
         concept empty = concepts::range< T> && 
         requires( T a) { { a.empty()} -> std::convertible_to< bool>; };

         template< typename T>
         concept back_inserter = concepts::range< T> && 
         requires( T a) { std::back_inserter( a); };

         template< typename T>
         concept sequence = concepts::range< T> && container::resize< T>;

         template< typename T>
         concept associative = concepts::range< T> && container::value::insert< T>;

         template< typename T>
         concept array = std::is_array_v< T> || ( concepts::range< T> && requires 
         {            
            std::tuple_size< T>::value;
         });

         template< typename T>
         concept like = container::sequence< T> || container::associative< T>;

      } // container

      namespace string
      {
         template< typename T>
         concept value_type = concepts::any_of< std::remove_cvref_t< T>, char>;

         template< typename T>
         concept like = concepts::range< T> && requires( const T& a)
         {
            { *std::begin( a)} -> string::value_type;
         };
         
      } // string

      namespace binary
      {
         template< typename T>
         concept value_type = concepts::any_of< std::remove_cvref_t< T>, char, unsigned char, signed char, std::byte>;

         template< typename T>
         concept iterator = std::contiguous_iterator< T> && binary::value_type< std::iter_value_t< T>>;

         template< typename T>
         concept like = concepts::range< T> && requires( const T& a)
         {
            { *std::begin( a)} -> binary::value_type; 
         };
         
      } // binary

      template< typename T, typename... Ts> 
      concept derived_from = ( std::derived_from< std::remove_cvref_t< T>, Ts> || ...);

      template< typename T>
      concept enumerator = std::is_enum_v< T>;
      
   } // concepts
} // casual
