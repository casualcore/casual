//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/platform.h"

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

         template< typename...>
         using try_to_instantiate = void;

         namespace detect
         {
            //
            // Taken pretty much straight of from N4502
            //

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
               //!
               //! @returns number of arguments
               //!
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


            };

         }

         namespace has
         {
            template< typename T> 
            using call_operator_exists = decltype( &T::operator());

            template< typename T>
            using call_operator = detect::is_detected< call_operator_exists, T>;
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

         //!
         //! const functor specialization
         //!
         template< typename C, typename R, typename ...Args>
         struct function< R(C::*)(Args...) const> : public detail::function< R, Args...>
         {
         };

         //!
         //! non const functor specialization
         //!
         template< typename C, typename R, typename ...Args>
         struct function< R(C::*)(Args...)> : public detail::function< R, Args...>
         {
         };

         //!
         //! free function specialization
         //!
         template< typename R, typename ...Args>
         struct function< R(*)(Args...)> : public detail::function< R, Args...>
         {
         };



         //!
         //! Removes cv and references
         //!
         template< typename T>
         struct remove_cvref
         {
            using type = std::remove_cv_t< std::remove_reference_t< T>>;
         };


         //!
         //! Removes cv and references
         //!
         template< typename T>
         using remove_cvref_t = typename remove_cvref< T>::type;



         namespace container
         {
            namespace category
            {
               struct container {};
               struct traversable : container {};

               struct associative : traversable {};
               struct unordered : associative {};

               struct sequence : traversable {};
               struct continuous : sequence {};

               
               struct native_array : continuous {};
               struct array : native_array {};

               struct adaptor : container{};

               struct string_like : continuous {};
               struct string_array : string_like {};
               struct string : string_like {};
            } // category

            namespace detail
            {
               template< typename Category>
               struct category_traits
               {
                  using category = Category;
               };

               template< typename Container, typename Category>
               struct traits : category_traits< Category>
               {
                  using iterator = decltype( std::begin( std::declval< Container&>()));
                  using tag = typename std::iterator_traits< iterator>::iterator_category;
               };

            } // detail

            template< typename T>
            struct traits { using category = void; using iterator = void; using tag = void;};



            template< typename T, std::size_t size>
            struct traits< std::array< T, size>> : detail::traits< std::array< T, size>, container::category::array>{};

            template< typename T, std::size_t size>
            struct traits< T[ size]> : detail::traits< T[ size], container::category::native_array>{};

            template< std::size_t size>
            struct traits< char[ size]> : detail::traits< char[ size], container::category::string_array>{};
            template< std::size_t size>
            struct traits< unsigned char[ size]> : detail::traits< unsigned char[ size], container::category::string_array>{};
            template< std::size_t size>
            struct traits< signed char[ size]> : detail::traits< signed char[ size], container::category::string_array>{};

            template<>
            struct traits< std::string> : detail::traits< std::string, container::category::string>{};

            template< typename T>
            struct traits< std::vector< T>> : detail::traits< std::vector< T>, container::category::continuous>{};
            template< typename T>
            struct traits< std::deque< T>> : detail::traits< std::deque< T>, container::category::sequence>{};
            template< typename T>
            struct traits< std::list< T>> : detail::traits< std::list< T>, container::category::sequence>{};


            template< typename T>
            struct traits< std::set< T>> : detail::traits< std::set< T>, container::category::associative>{};
            template<  typename K, typename V>
            struct traits< std::map< K, V>> : detail::traits< std::map< K, V>, container::category::associative>{};
            template< typename T>
            struct traits< std::multiset< T>> : detail::traits< std::multiset< T>, container::category::associative>{};
            template<  typename K, typename V>
            struct traits< std::multimap< K, V>> : detail::traits< std::multimap< K, V>, container::category::associative>{};

            template< typename T>
            struct traits< std::unordered_set< T>> : detail::traits< std::unordered_set< T>, container::category::unordered>{};
            template<  typename K, typename V>
            struct traits< std::unordered_map< K, V>> : detail::traits< std::unordered_map< K, V>, container::category::unordered>{};
            template< typename T>
            struct traits< std::unordered_multiset< T>> : detail::traits< std::unordered_multiset< T>, container::category::unordered>{};
            template<  typename K, typename V>
            struct traits< std::unordered_multimap< K, V>> : detail::traits< std::unordered_multimap< K, V>, container::category::unordered>{};


            template< typename T, typename Container>
            struct traits< std::stack< T, Container>> : detail::category_traits< container::category::adaptor>{};
            template< typename T, typename Container>
            struct traits< std::queue< T, Container>> : detail::category_traits< container::category::adaptor>{};
            template< typename T, typename Container>
            struct traits< std::priority_queue< T, Container>> : detail::category_traits< container::category::adaptor>{};

            template< typename T>
            using category_t = typename traits< T>::category;


            template< typename Container, typename Category>
            struct is_category : bool_constant<
               std::is_base_of< Category, category_t< remove_cvref_t< Container>>>::value> {};

            template< typename Container>
            struct is_container : is_category< Container, category::container> {};

            template< typename Container>
            struct is_associative : is_category< Container, category::associative> {};

            template< typename Container>
            struct is_unordered : is_category< Container, category::unordered> {};

            template< typename Container>
            struct is_sequence : is_category< Container, category::sequence> {};

            template< typename Container>
            struct is_adaptor : is_category< Container, category::adaptor> {};

            template< typename Container>
            struct is_array : is_category< Container, category::array> {};

            template< typename Container>
            struct is_string : is_category< Container, category::string_like> {};


         } // container


         namespace iterator
         {
            //!
            //! SFINAE friendly iterator_traits
            //! @{

            template< typename T>
            using has_category = typename std::iterator_traits< std::remove_reference_t< T>>::iterator_category;

            template< typename  T, bool = detect::is_detected< has_category, T>::value>
            struct traits : std::iterator_traits< std::remove_reference_t< T>> {};

            template<class T>
            struct traits<T, false> {};

            //! @}

            template< typename T>
            using has_reference = typename std::iterator_traits< std::remove_reference_t< T>>::reference;

            namespace detail
            {
               template< typename Iter, typename Tag, bool = detect::is_detected< has_category, Iter>::value>
               struct is_tag : bool_constant<
                  std::is_base_of< Tag, typename iterator::traits< std::remove_reference_t< Iter>>::iterator_category>::value> {};

               template< typename Iter, typename Tag>
               struct is_tag< Iter, Tag, false> : std::false_type {};
            }


            template< typename Iter>
            struct is_random_access : detail::is_tag< Iter, std::random_access_iterator_tag> {};

            template< typename Iter>
            struct is_output : bool_constant<
               ( 
                  detail::is_tag< Iter, std::output_iterator_tag>::value
                  || 
                  (
                     detail::is_tag< Iter, std::forward_iterator_tag>::value
                     && ! std::is_const< detect::detected_t< has_reference, Iter>>::value
                  )
               )
               > {};

         } // iterator

         template< typename T> 
         struct type 
         {
            constexpr static bool nothrow_construct = std::is_nothrow_constructible< T>::value;
            constexpr static bool nothrow_move_assign = std::is_nothrow_move_assignable< T>::value;
            constexpr static bool nothrow_move_construct = std::is_nothrow_move_constructible< T>::value;
            constexpr static bool nothrow_copy_assign = std::is_nothrow_copy_assignable< T>::value;
            constexpr static bool nothrow_copy_construct = std::is_nothrow_copy_constructible< T>::value;
         };


#if __GNUC__ > 4 || __clang_major__ > 4

         template< typename T>
         using is_trivially_copyable = std::is_trivially_copyable< T>;
#else
         //!
         //!  std::is_trivially_copyable is not implemented with gcc 4.8.3
         //!
         template< typename T>
         struct is_trivially_copyable : bool_constant< true> {};


#endif

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

         namespace concrete
         {
            template< typename T>
            using type_t = typename std::remove_cv< typename std::remove_reference< T>::type>::type;

            template< typename E>
            constexpr auto type( E&& expression) -> type_t< decltype( std::forward< E>( expression))>
            {
               return {};
            }

         } // expression

         template< typename T>
         struct is_by_value_friendly : bool_constant< 
            is_trivially_copyable< remove_cvref_t< T>>::value
            && sizeof( remove_cvref_t< T>) <= platform::size::by::value::max> {};


         template< typename T>
         struct by_value_or_const_ref : std::conditional< is_by_value_friendly< T>::value, remove_cvref_t< T>, const T&> {};

         template< typename T>
         using by_value_or_const_ref_t = typename by_value_or_const_ref< T>::type;


#if __cplusplus > 201402L // vector will have nothrow move in c++17
         template< typename T>
         struct is_movable : bool_constant<
            std::is_nothrow_move_constructible< T>::value && std::is_nothrow_move_assignable< T>::value> {};
         
#else
         //!
         //!  containers and std::string is not noexcept movable with gcc 4.9.x
         //!
         template< typename T>
         struct is_movable : bool_constant<
            std::is_move_constructible< T>::value && std::is_move_assignable< T>::value> {};

#endif





         //!
         //! Arbitrary number of types to compare if same
         //!
         //! @{
         template< typename T1, typename T2, typename... Args>
         struct is_same : bool_constant< is_same< T1, T2>::value && is_same< T2, Args...>::value>
         {

         };

         template< typename T1, typename T2>
         struct is_same< T1, T2> : std::is_same< T1, T2>
         {
         };
         //! @}

         //!
         //! Answer the question if T is the same type as any of the options
         //! 
         //! @{
         template< typename T, typename Option, typename... Options>
         struct is_any : bool_constant< is_same< T, Option>::value || is_any< T, Options...>::value>
         {

         };

         template< typename T, typename Option>
         struct is_any< T, Option> : std::is_same< T, Option>
         {
         };
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

         namespace is
         {
            namespace detail
            {
               template< typename T> 
               using function = decltype( function< T>::arguments());

               template< typename T>
                using tuple = decltype( std::tuple_size< T>::value);

               template< typename T> 
               using optional = decltype( std::declval< T&>().has_value());

               template< typename T>
               using iterable = std::tuple< decltype( std::begin( std::declval< T&>())), decltype( std::end( std::declval< T&>()))>;

               //template< typename T> 
               //using iterator_value_t = detect::detected_or_t< decltype( std::begin( std::declval< T&>()))

               template< typename T>
               using iterator_value = decltype( *std::begin( std::declval< T&>()));
               
            } // detail

            template< typename T>
            using function = detect::is_detected< detail::function, T>;




            template< typename T>
            using tuple = detect::is_detected< detail::tuple, T>;

            static_assert( is::tuple< std::tuple< int, int>>::value, "");
            static_assert( ! is::tuple< int>::value, "");


            template< typename T>
            using optional_like = detect::is_detected< detail::optional, T>;


            template< typename T>
            using iterable = detect::is_detected< detail::iterable, T>;

            template< typename T>
            using iterator = detect::is_detected< traits::iterator::has_category, T>;
            
            template< typename T>
            using char_type = bool_constant< is_any< 
               remove_cvref_t< T>, char, unsigned char, signed char>::value>;

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



            namespace reverse
            {
               namespace detail
               {
                  template< typename T>
                  using iterable = std::tuple< decltype( std::declval< T&>().rbegin()), decltype( std::declval< T&>().rend())>;                  
               } // detail


               template< typename T>
               using iterable = detect::is_detected< detail::iterable, T>;
            }
         } // is

         namespace member
         {
            template< typename T>
            using has_size = decltype( std::declval< T&>().size());

            template< typename T>
            using has_empty = decltype( std::declval< T&>().empty());

            template< typename T>
            using has_insert = decltype( std::declval< T&>().insert( std::begin( std::declval< T&>()), std::begin( std::declval< T&>()), std::begin( std::declval< T&>())));

            template< typename T>
            using has_push_back = decltype( std::declval< T&>().push_back( *std::begin( std::declval< T&>())));

            template< typename T, typename A>
            using has_serialize = decltype( std::declval< T&>().serialize( std::declval< A&>()));
         }

         namespace has
         {
            template< typename T>
            using size = detect::is_detected< member::has_size, T>;

            template< typename T>
            using empty = detect::is_detected< member::has_empty, T>;


            template< typename T>
            using insert = detect::is_detected< member::has_insert, T>;


            template< typename T>
            using push_back = detect::is_detected< member::has_push_back, T>;

            template< typename T, typename A>
            using serialize = detect::is_detected< member::has_serialize, T, A>;

            namespace detail
            {
               template< typename T> 
               using ostream_stream_operator = decltype( std::declval< std::ostream&>() << std::declval< T&>()); 
            } // detail


            template< typename T>
            using ostream_stream_operator = detect::is_detected< detail::ostream_stream_operator, T>;
         }

      } // traits
   } // common
} // casual


