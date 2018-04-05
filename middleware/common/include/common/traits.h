//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef COMMON_TRAITS_H_
#define COMMON_TRAITS_H_

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
         template< typename...>
         using void_t = void;

         namespace detect
         {
            //
            // Taken pretty much straight of from N4502
            //

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

            template< template<class...> class Op, class... Args >
            using is_detected = typename detector<void, void, Op, Args...>::value_t;

            template< template<class...> class Op, class... Args >
            using detected_t = typename detector<void, void, Op, Args...>::type;
         }

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
         struct basic_type
         {
            using type = std::remove_reference_t< std::remove_cv_t< T>>;
         };


         //!
         //! Removes cv and references
         //!
         template< typename T>
         using basic_type_t = typename basic_type< T>::type;



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

               struct array : continuous {};

               struct adaptor : container{};

               struct string : continuous{};
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
                  using iterator = decltype( std::begin( std::declval< Container>()));
                  using tag = typename std::iterator_traits< iterator>::iterator_category;
               };

            } // detail

            template< typename T>
            struct traits { using category = void; using iterator = void; using tag = void;};



            template< typename T, std::size_t size>
            struct traits< std::array< T, size>> : detail::traits< std::array< T, size>, container::category::array>{};

            template< typename T, std::size_t size>
            struct traits< T[ size]> : detail::traits< std::array< T, size>, container::category::array>{};



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
            struct is_category : std::integral_constant< bool,
               std::is_base_of< Category, category_t< basic_type_t< Container>>>::value> {};

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
            struct is_string : is_category< Container, category::string> {};

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
               struct is_tag : std::integral_constant< bool,
                  std::is_base_of< Tag, typename iterator::traits< std::remove_reference_t< Iter>>::iterator_category>::value> {};

               template< typename Iter, typename Tag>
               struct is_tag< Iter, Tag, false> : std::false_type {};
            }


            template< typename Iter>
            struct is_random_access : detail::is_tag< Iter, std::random_access_iterator_tag> {};

            template< typename Iter>
            struct is_output : std::integral_constant< bool,
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



#if __GNUC__ > 4 || __clang_major__ > 4

         template< typename T>
         using is_trivially_copyable = std::is_trivially_copyable< T>;
#else
         //!
         //!  std::is_trivially_copyable is not implemented with gcc 4.8.3
         //!
         template< typename T>
         struct is_trivially_copyable : std::integral_constant< bool, true> {};


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



#if __cplusplus > 201402L // vector will have nothrow move in c++17
         template< typename T>
         struct is_movable : std::integral_constant< bool,
            std::is_nothrow_move_constructible< T>::value && std::is_nothrow_move_assignable< T>::value> {};
#else
         //!
         //!  containers and std::string is not noexcept movable with gcc 4.9.x
         //!
         template< typename T>
         struct is_movable : std::integral_constant< bool,
            std::is_move_constructible< T>::value && std::is_move_assignable< T>::value> {};
#endif





         //!
         //! Arbitrary number of types to compare if same
         //!
         //! @{
         template< typename T1, typename T2, typename... Args>
         struct is_same : std::integral_constant< bool, is_same< T1, T2>::value && is_same< T2, Args...>::value>
         {

         };

         template< typename T1, typename T2>
         struct is_same< T1, T2> : std::is_same< T1, T2>
         {
         };
         //! @}


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
            template< typename T> 
            using detail_function = decltype( function< T>::arguments());
            template< typename T>
            using function = detect::is_detected< detail_function, T>;

            template< typename T>
            using detail_tuple = decltype( std::tuple_size< T>::value);
            template< typename T>
            using tuple = detect::is_detected< detail_tuple, T>;

            static_assert( is::tuple< std::tuple< int, int>>::value, "");
            static_assert( ! is::tuple< int>::value, "");

            template< typename T> 
            using detail_optional = decltype( std::declval< T&>().has_value());

            template< typename T>
            using optional_like = detect::is_detected< detail_optional, T>;


            template< typename T>
            using begin_end_existing = std::tuple< decltype( std::begin( std::declval< T&>())), decltype( std::end( std::declval< T&>()))>;

            template< typename T>
            using iterable = detect::is_detected< begin_end_existing, T>;

            template< typename T>
            using iterator = detect::is_detected< traits::iterator::has_category, T>;


            namespace reverse
            {
               template< typename T>
               using has_rbegin_rend = std::tuple< decltype( std::declval< T&>().rbegin()), decltype( std::declval< T&>().rend())>;

               template< typename T>
               using iterable = detect::is_detected< has_rbegin_rend, T>;
            }
         }

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

         }

      } // traits
   } // common
} // casual

#endif // TRAITS_H_
