//!
//! casual
//!

#ifndef COMMON_TRAITS_H_
#define COMMON_TRAITS_H_

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

               using result_type = R;

               template< std::size_t index>
               struct argument
               {
                  using type = typename std::tuple_element< index, std::tuple< Args...>>::type;
               };
            };

         }

         template<typename T>
         struct function : public function< decltype( &T::operator())>
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
         struct function< R(Args...)> : public detail::function< R, Args...>
         {
         };



         //!
         //! Removes cv and references
         //!
         template< typename T>
         struct basic_type
         {
            using type = typename std::remove_reference< typename std::remove_cv< T>::type>::type;
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



            template< typename T>
            struct traits< std::vector< T>> : detail::traits< std::vector< T>, container::category::continuous>{};
            template< typename T>
            struct traits< std::deque< T>> : detail::traits< std::deque< T>, container::category::sequence>{};
            template< typename T>
            struct traits< std::list< T>> : detail::traits< std::list< T>, container::category::sequence>{};

            //template< typename T>
            //struct traits< std::forward_list< T>> : detail::traits< std::forward_list< T>, container::category::sequence>{};


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

         } // container


         namespace iterator
         {

            template< typename Iter, typename Tag>
            struct is_tag : std::integral_constant< bool,
               std::is_base_of< Tag, typename std::iterator_traits< Iter>::iterator_category>::value> {};


            template< typename Iter>
            struct is_random_access : is_tag< Iter, std::random_access_iterator_tag> {};

            template< typename Iter>
            struct is_output : std::integral_constant< bool,
               is_tag< Iter, std::output_iterator_tag>::value
               || (
                     is_tag< Iter, std::forward_iterator_tag>::value
                     && ! std::is_const< typename std::iterator_traits< Iter>::reference>::value
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

         template< typename T>
         using decay_t = typename std::decay< T>::type;

         template< class T >
         using remove_reference_t = typename std::remove_reference< T>::type;



#if __GNUC__ > 4 || __clang_major__ > 4
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

         template< bool Predicate, typename V = void>
         using enable_if_t = typename std::enable_if< Predicate, V>::type;


      } // traits
   } // common
} // casual

#endif // TRAITS_H_
