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
   namespace common
   {
      namespace traits
      {

         template <bool B>
         using bool_constant = std::integral_constant<bool, B>;

         template< typename...>
         using void_t = void;

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
               struct detector<Default, void_t<Op<Args...>>, Op, Args...>
               {
                  using value_t = std::true_type;
                  using type = Op<Args...>;
               };

            } // detail

            template< template<class...> class Op, class... Args >
            using is_detected = typename detail::detector<void, void, Op, Args...>::value_t;

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
            constexpr static bool nothrow_construct = std::is_nothrow_constructible< T>::value;
            constexpr static bool nothrow_move_assign = std::is_nothrow_move_assignable< T>::value;
            constexpr static bool nothrow_move_construct = std::is_nothrow_move_constructible< T>::value;
            constexpr static bool nothrow_copy_assign = std::is_nothrow_copy_assignable< T>::value;
            constexpr static bool nothrow_copy_construct = std::is_nothrow_copy_constructible< T>::value;
         };


         template< typename T>
         using is_trivially_copyable = std::is_trivially_copyable< T>;

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
            is_trivially_copyable< remove_cvref_t< T>>::value
            && sizeof( remove_cvref_t< T>) <= platform::size::by::value::max> {};


         template< typename T>
         struct by_value_or_const_ref : std::conditional< is_by_value_friendly< T>::value, remove_cvref_t< T>, const T&> {};

         template< typename T>
         using by_value_or_const_ref_t = typename by_value_or_const_ref< T>::type;

         template< typename T>
         struct is_nothrow_movable : bool_constant<
            std::is_nothrow_move_constructible< T>::value && std::is_nothrow_move_assignable< T>::value> {}; 

#if __cplusplus > 201402L // vector will have nothrow move in c++17
         template< typename T>
         using is_movable = is_nothrow_movable< T>;
         
#else
         //!  containers and std::string is not noexcept movable with gcc 4.9.x
         template< typename T>
         struct is_movable : bool_constant<
            std::is_move_constructible< T>::value && std::is_move_assignable< T>::value> {};
#endif

 

         template< typename T>
         struct is_copyable : bool_constant< 
            std::is_copy_constructible< T>::value && std::is_copy_assignable< T>::value> {};




         //! Arbitrary number of types to compare if same
         //! @{
         template< typename T1, typename T2, typename... Args>
         struct is_same : bool_constant< is_same< T1, T2>::value && is_same< T2, Args...>::value> {};

         template< typename T1, typename T2>
         struct is_same< T1, T2> : std::is_same< T1, T2> {};
         //! @}

         namespace concrete
         {
            //! Arbitrary number of types to compare if the `concrete` type is the same
            //! @{
            template< typename T1, typename T2, typename... Args>
            struct is_same : bool_constant< is_same< T1, T2>::value && is_same< T2, Args...>::value> {};

            template< typename T1, typename T2>
            struct is_same< T1, T2> : std::is_same< remove_cvref_t< T1>, remove_cvref_t< T2>> {};
         //! @}
            
         } // concrete

         //! Answer the question if T is the same type as any of the options
         //! @{
         template< typename T, typename Option, typename... Options>
         struct is_any : bool_constant< is_same< T, Option>::value || is_any< T, Options...>::value> {};

         template< typename T, typename Option>
         struct is_any< T, Option> : std::is_same< T, Option> {};
         //! @}

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
            using size = detect::is_detected< detail::size, T>;

            template< typename T>
            using resize = detect::is_detected< detail::resize, T>;

            template< typename T>
            using empty = detect::is_detected< detail::empty, T>;

            template< typename T>
            using insert = detect::is_detected< detail::insert, T>;

            namespace value
            {
               template< typename T>
               using insert = detect::is_detected< detail::value::insert, T>;
            } // value

            template< typename T>
            using push_back = detect::is_detected< detail::push_back, T>;

            template< typename T>
            using ostream_stream_operator = detect::is_detected< detail::ostream_stream_operator, T>;

            template< typename T>
            using flush = detect::is_detected< detail::flush, T>;
            
            namespace tuple
            {
               template< typename T>
               using size = detect::is_detected< detail::tuple::size, T>;
            } // tuple
            
            namespace any
            {
               template< typename T, typename Option, typename... Options>
               struct base : bool_constant< has::any::base< T, Option>::value || has::any::base< T, Options...>::value> {};

               template< typename T, typename Option>
               struct base< T, Option> : std::is_base_of< Option, T> {};
            } // any

         } // has

         namespace is
         {
            namespace detail
            {
               template< typename T> 
               using function = decltype( function< T>::arguments());

               template< typename T> 
               using optional = decltype( std::declval< T&>().has_value());

               template< typename T>
               using iterable = std::tuple< decltype( std::begin( std::declval< T&>())), decltype( std::end( std::declval< T&>()))>;

               template< typename T> 
               using iterator = typename std::iterator_traits< std::remove_reference_t< T>>::iterator_category;

               template< typename T>
               using iterator_value = decltype( *std::begin( std::declval< T&>()));

               namespace output
               {
                  template< typename T> 
                  using iterator = decltype( *( std::declval< T&>()++) = *std::declval< T&>());
                  
               } // output
    
            } // detail

            template< typename T>
            using function = detect::is_detected< detail::function, T>;

            template< typename T>
            using iterable = detect::is_detected< detail::iterable, T>;

            template< typename T>
            using iterator = detect::is_detected< detail::iterator, T>;

            template< typename T>
            using tuple = bool_constant< has::tuple::size< T>::value && ! is::iterable< T>::value>;

            static_assert( is::tuple< std::tuple< int, int>>::value, "");
            static_assert( is::tuple< std::pair< int, int>>::value, "");
            static_assert( ! is::tuple< int>::value, "");


            template< typename T>
            using optional_like = detect::is_detected< detail::optional, T>;



            namespace output
            {
               template< typename T>
               using iterator = detect::is_detected< detail::output::iterator, T>;
               
            } // output
            
            template< typename T>
            using char_type = bool_constant< is_any< 
               remove_cvref_t< T>, char, unsigned char, signed char>::value>;

            template< typename T>
            using char_pointer = bool_constant< std::is_pointer< T>::value 
               && is::char_type< decltype( *T())>::value>;

            namespace string
            {
               template< typename T>
               using like = bool_constant< is::iterable< T>::value 
                  && is::char_type< decltype( *std::begin( std::declval< T&>()))>::value>;

               template< typename T>
               using iterator = bool_constant< is::iterator< T>::value 
                  && is::char_type< decltype( *std::declval< T>())>::value>;
            } // string



            namespace binary
            {
               template< typename T>
               using value = char_type< T>;

               template< typename T>
               using iterator = bool_constant< is::iterator< T>::value 
                  && is::binary::value< decltype( *std::declval< T>())>::value>;

               template< typename T>
               using like = bool_constant< is::iterable< T>::value 
                  && is::binary::value< detect::detected_t< detail::iterator_value, T>>::value>;
            } // binary


            namespace container
            {
               namespace sequence
               {
                  template< typename T>
                  using like = bool_constant< is::iterable< T>::value 
                     && has::resize< T>::value>;
                  
               } // sequence

               namespace associative
               {
                  template< typename T>
                  using like = bool_constant< is::iterable< T>::value 
                     && has::value::insert< T>::value>;
                  
               } // associative

               namespace array
               {
                  template< typename T>
                  using like = bool_constant< 
                     std::is_array< T>::value 
                     || ( is::iterable< T>::value && has::tuple::size< T>::value)
                  >;
               } // fixed

               template< typename T>
               using like = bool_constant< is::container::sequence::like< T>::value 
                  || is::container::associative::like< T>::value>;
            } // container

            namespace reverse
            {
               namespace detail
               {
                  template< typename T>
                  using iterable = std::tuple< decltype( std::declval< T&>().rbegin()), decltype( std::declval< T&>().rend())>;                  
               } // detail

               template< typename T>
               using iterable = detect::is_detected< detail::iterable, T>;
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

      } // traits
   } // common
} // casual


