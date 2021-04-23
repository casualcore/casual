//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "casual/platform.h"

#include <type_traits>


#include <vector>
#include <deque>
#include <list>

#include <set>
#include <map>

#include <stack>
#include <queue>

#include <unordered_map>
#include <unordered_set>

namespace casual
{
   namespace common::traits
   {
      template <bool B>
      using bool_constant = std::integral_constant<bool, B>;

      namespace priority
      {
         template< size_t I> 
         struct tag : tag<I-1> {};
         
         template<> 
         struct tag< 0> {};
      } // priority

      namespace detect
      {
         // Taken pretty much straight of from N4502

         namespace detail
         {
            
            // primary template handles all types not supporting the archetypal Op:
            template< typename Default 
               , typename // always void; supplied externally
               , template< typename...> class Op
               , typename... Args>
            struct detector
            {
               using value_t = std::false_type;
               using type = Default;
            };

            // the specialization recognizes and handles only types supporting Op:
            template< typename Default
            , template< typename...> class Op
            , typename... Args
            >
            struct detector< Default, std::void_t< Op< Args...>>, Op, Args...>
            {
               using value_t = std::true_type;
               using type = Op<Args...>;
            };

         } // detail

         template< template<class...> class Op, class... Args>
         using is_detected = typename detail::detector<void, void, Op, Args...>::value_t;

         template< template<class...> class Op, class... Args>
         constexpr bool is_detected_v = is_detected< Op, Args...>::value;

         template< template<class...> class Op, class... Args >
         using detected_t = typename detail::detector<void, void, Op, Args...>::type;

         template< typename Default, template<class...> class Op, class... Args>
         using detected_or = detail::detector< Default, void, Op, Args...>;

         template< typename Default, template<class...> class Op, class... Args>
         using detected_or_t = typename detail::detector< Default, void, Op, Args...>::type;
      } // detect

      template< typename... Args>
      struct pack{};

      namespace detail
      {

         template< typename R, typename ...Args>
         struct function
         {
            //! @returns number of arguments
            constexpr static auto arguments() -> decltype( sizeof...(Args))
            {
               return sizeof...(Args);
            }

            using decayed = std::tuple< std::decay_t< Args>...>;

            constexpr static auto tuple() { return std::tuple< std::decay_t< Args>...>{}; }

            using result_type = R;

            template< platform::size::type index>
            struct argument
            {
               using type = std::tuple_element_t< index, std::tuple< Args...>>;
            };

            template< platform::size::type index>
            using argument_t = typename argument< index>::type;
         };
      }

      namespace has
      {
         namespace detail
         {
            template< typename T> 
            using call_operator = decltype( &T::operator());               
         } // detail

         template< typename T>
         using call_operator = detect::is_detected< detail::call_operator, T>;
      }

      template<typename T, typename Enable = void>
      struct function {};

      template<typename T>
      struct function< T, std::enable_if_t< has::call_operator< T>::value>> 
         : public function< decltype( &T::operator())>
      {
      };

      template<typename T>
      struct function< std::reference_wrapper< T>> : public function< T>
      {
      };

      //! const functor specialization
      template< typename C, typename R, typename ...Args>
      struct function< R(C::*)(Args...) const> : public detail::function< R, Args...>
      {
      };

      //! non const functor specialization
      template< typename C, typename R, typename ...Args>
      struct function< R(C::*)(Args...)> : public detail::function< R, Args...>
      {
      };

      //! free function specialization
      template< typename R, typename ...Args>
      struct function< R(*)(Args...)> : public detail::function< R, Args...>
      {
      };


      //! Removes cv and references
      template< typename T>
      struct remove_cvref
      {
         using type = std::remove_cv_t< std::remove_reference_t< T>>;
      };

      //! Removes cv and references
      template< typename T>
      using remove_cvref_t = typename remove_cvref< T>::type;


      template< typename T> 
      struct type 
      {
         constexpr static bool nothrow_default_constructible = std::is_nothrow_default_constructible_v< T>;
         constexpr static bool nothrow_construct = std::is_nothrow_constructible_v< T>;
         constexpr static bool nothrow_move_assign = std::is_nothrow_move_assignable_v< T>;
         constexpr static bool nothrow_move_construct = std::is_nothrow_move_constructible_v< T>;
         constexpr static bool nothrow_copy_assign = std::is_nothrow_copy_assignable_v< T>;
         constexpr static bool nothrow_copy_construct = std::is_nothrow_copy_constructible_v< T>;
      };

      //!
      //! SFINAE friendly underlying_type
      //! @{

      template<class T, bool = std::is_enum< T>::value>
      struct underlying_type : std::underlying_type<T> {};

      template<class T>
      struct underlying_type<T, false> {};

      template< typename T>
      using underlying_type_t = typename underlying_type< T>::type;
      //! @}


      template< typename T>
      struct is_by_value_friendly : bool_constant< 
         std::is_trivially_copyable_v< remove_cvref_t< T>>
         && sizeof( remove_cvref_t< T>) <= platform::size::by::value::max> {};


      template< typename T>
      struct by_value_or_const_ref : std::conditional< is_by_value_friendly< T>::value, remove_cvref_t< T>, const T&> {};

      template< typename T>
      using by_value_or_const_ref_t = typename by_value_or_const_ref< T>::type;

      template< typename T>
      struct is_nothrow_movable : bool_constant<
         std::is_nothrow_move_constructible< T>::value && std::is_nothrow_move_assignable< T>::value> {}; 

      template< typename T>
      using is_movable = is_nothrow_movable< T>;

      //! usable in else branch in constexpr if context
      template< typename T> 
      struct dependent_false : std::false_type {};

      /* not needed right now, but could be... 
      //!
      //! 
      //! defines value == true if all types differ, otherwise false
      //!
      //! @{
      template< typename T1, typename T2, typename... Args>
      struct is_different : bool_constant< is_different< T1, T2>::value && is_different< T2, Args...>::value>
      {

      };

      template< typename T1, typename T2>
      struct is_different< T1, T2> : bool_constant< ! std::is_same< T1, T2>::value>
      {
      };
      //! @}
      */

      template< template< typename> class Predicate, typename T, typename... Ts>
      struct any_of : bool_constant< Predicate< T>::value || any_of< Predicate, Ts...>::value> {};

      template< template< typename> class Predicate, typename T>
      struct any_of< Predicate, T> : bool_constant< Predicate< T>::value> {};

      struct unmovable
      {
         unmovable() = default;
         unmovable( unmovable&&) = delete;
         unmovable& operator = ( unmovable&&) = delete;
      };

      struct uncopyable
      {
         uncopyable() = default;
         uncopyable( const uncopyable&) = delete;
         uncopyable& operator = ( const uncopyable&) = delete;
      };

      struct unrelocatable : unmovable, uncopyable {};


      namespace has
      {
         namespace detail
         {
            template< typename T>
            using size = decltype( std::declval< T&>().size());

            template< typename T>
            using resize = decltype( std::declval< T&>().resize( size< T>()));

            template< typename T>
            using reserve = decltype( std::declval< T&>().reserve( size< T>()));

            template< typename T>
            using empty = decltype( std::declval< T&>().empty());

            template< typename T>
            using insert = decltype( std::declval< T&>().insert( std::begin( std::declval< T&>()), std::begin( std::declval< T&>()), std::begin( std::declval< T&>())));

            template< typename T>
            using push_back = decltype( std::declval< T&>().push_back( *std::begin( std::declval< T&>())));

            template< typename T> 
            using ostream_stream_operator = decltype( std::declval< std::ostream&>() << std::declval< T&>());

            namespace value
            {
               template< typename T>
               using insert = decltype( std::declval< T&>().insert( *std::begin( std::declval< T&>())));
            } // value

            template< typename T>
            using flush = decltype( std::declval< T&>().flush());

            namespace tuple
            {
               template< typename T>
               using size = decltype( std::tuple_size< T>::value);
            } // tuple

         } // detail

         template< typename T>
         inline constexpr bool size_v = detect::is_detected_v< detail::size, T>;

         template< typename T>
         inline constexpr bool resize_v = detect::is_detected_v< detail::resize, T>;

         template< typename T>
         inline constexpr bool reserve_v = detect::is_detected_v< detail::reserve, T>;

         template< typename T>
         inline constexpr bool empty_v = detect::is_detected_v< detail::empty, T>;

         template< typename T>
         inline constexpr bool insert_v = detect::is_detected_v< detail::insert, T>;

         namespace value
         {
            template< typename T>
            inline constexpr bool insert_v = detect::is_detected_v< detail::value::insert, T>;
         } // value

         template< typename T>
         inline constexpr bool push_back_v = detect::is_detected_v< detail::push_back, T>;

         template< typename T>
         inline constexpr bool ostream_stream_operator_v = detect::is_detected_v< detail::ostream_stream_operator, T>;

         template< typename T>
         inline constexpr bool flush_v = detect::is_detected_v< detail::flush, T>;
         
         namespace tuple
         {
            template< typename T>
            inline constexpr bool size_v = detect::is_detected_v< detail::tuple::size, T>;
         } // tuple
         
         namespace any
         {
            template< typename T, typename Option, typename... Options>
            struct base : bool_constant< has::any::base< T, Option>::value || has::any::base< T, Options...>::value> {};

            template< typename T, typename Option>
            struct base< T, Option> : std::is_base_of< Option, T> {};

            template< typename T, typename... Ts>
            inline constexpr bool base_v = base< T, Ts...>::value;
         } // any

      } // has

      namespace is
      {
         namespace detail
         {
            template< typename T1, typename T2, typename... Args>
            struct same : bool_constant< same< T1, T2>::value && same< T2, Args...>::value> {};

            template< typename T1, typename T2>
            struct same< T1, T2> : std::is_same< T1, T2> {};

            template< typename T, typename Option, typename... Options>
            struct any : bool_constant< std::is_same< T, Option>::value || any< T, Options...>::value> {};

            template< typename T, typename Option>
            struct any< T, Option> : std::is_same< T, Option> {};

            template< typename T> 
            using function = decltype( function< T>::arguments());

            template< typename T> 
            using optional = decltype( std::declval< T&>().has_value());

            template< typename T>
            using iterable = std::tuple< decltype( std::begin( std::declval< T&>())), decltype( std::end( std::declval< T&>()))>;

            template< typename T> 
            using iterator = typename std::iterator_traits< std::remove_reference_t< T>>::iterator_category;

            template< typename T>
            using begin_dereferenced = decltype( *std::begin( std::declval< T&>()));

            template< typename T>
            using dereferenced = decltype( *std::declval< T>());

            namespace output
            {
               template< typename T> 
               using iterator = decltype( *( std::declval< T&>()++) = *std::declval< T&>());
               
            } // output
   
         } // detail


         template< typename T>
         inline constexpr bool copyable_v = std::is_copy_constructible_v< T> && std::is_copy_assignable_v< T>;

         //! Arbitrary number of types to compare if same
         template< typename T1, typename T2, typename... Ts>
         inline constexpr bool same_v = detail::same< T1, T2, Ts...>::value;

         //! Answer the question if T is the same type as any of the other types
         template< typename T1, typename T2, typename... Ts>
         inline constexpr bool any_v = detail::any< T1, T2, Ts...>::value;

         template< typename T>
         inline constexpr bool function_v = detect::is_detected_v< detail::function, T>;

         template< typename T>
         inline constexpr bool iterable_v = detect::is_detected_v< detail::iterable, T>;

         template< typename T>
         inline constexpr bool iterator_v = detect::is_detected_v< detail::iterator, T>;

         template< typename T>
         inline constexpr bool tuple_v = has::tuple::size_v< T> && ! is::iterable_v< T>;

         static_assert( is::tuple_v< std::tuple< int, int>>, "");
         static_assert( is::tuple_v< std::pair< int, int>>, "");
         static_assert( ! is::tuple_v< int>, "");


         template< typename T>
         inline constexpr bool optional_like_v = detect::is_detected_v< detail::optional, T>;


         namespace output
         {
            template< typename T>
            inline constexpr bool iterator_v = detect::is_detected_v< detail::output::iterator, T>;
            
         } // output


         template< typename T>
         inline constexpr bool char_type_v = is::any_v< remove_cvref_t< T>, char, unsigned char, signed char>;

         template< typename T>
         inline constexpr bool char_pointer_v = std::is_pointer_v< T> && is::char_type_v< decltype( *T())>;

         namespace string
         {
            template< typename T>
            inline constexpr bool like_v = is::iterable_v< T> && is::char_type_v< detect::detected_t< detail::begin_dereferenced, T>>;

            template< typename T>
            inline constexpr bool iterator_v = is::iterator_v< T> && is::char_type_v< detect::detected_t< detail::dereferenced, T>>;
         } // string



         namespace binary
         {
            template< typename T>
            inline constexpr bool value_v = char_type_v< T>;

            template< typename T>
            inline constexpr bool iterator_v = is::iterator_v< T> && is::binary::value_v< detect::detected_t< detail::dereferenced, T>>;

            template< typename T>
            inline constexpr bool like_v = is::iterable_v< T> && is::binary::value_v< detect::detected_t< detail::begin_dereferenced, T>>;
         } // binary


         namespace container
         {
            namespace sequence
            {
               template< typename T>
               inline constexpr bool like_v = is::iterable_v< T> && has::resize_v< T>;
               
            } // sequence

            namespace associative
            {
               template< typename T>
               inline constexpr bool like_v = is::iterable_v< T> && has::value::insert_v< T>;
               
            } // associative

            namespace array
            {
               template< typename T>
               inline constexpr bool like_v = std::is_array_v< T> || ( is::iterable_v< T> && has::tuple::size_v< T>);
            } // fixed

            template< typename T>
            inline constexpr bool like_v = is::container::sequence::like_v< T> || is::container::associative::like_v< T>;
         } // container

         namespace reverse
         {
            namespace detail
            {
               template< typename T>
               using iterable = std::tuple< decltype( std::declval< T&>().rbegin()), decltype( std::declval< T&>().rend())>;                  
            } // detail

            template< typename T>
            inline constexpr bool iterable_v = detect::is_detected_v< detail::iterable, T>;
         } // reverse
         
      } // is

      namespace iterable
      {
         namespace detail
         {
            template< typename T> 
            using type = remove_cvref_t< decltype( *( std::begin( std::declval< T&>())))>;
         } // detail

         template< typename T> 
         using value_t = detect::detected_t< detail::type, T>;
      } // iterable

      //! should be called `common::type` but the name clashes on 
      //! 'common' is to severe.
      namespace convertable
      {
         template< typename... Ts>
         constexpr auto type( Ts&&... ts) -> std::common_type_t< Ts...>;
      } // convertable


   } // common::traits
} // casual


