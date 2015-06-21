//!
//! traits.h
//!
//! Created on: Jun 29, 2014
//!     Author: Lazan
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


         template<typename T>
         struct function : public function< decltype( &T::operator())>
         {

         };


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



         template< typename T>
         struct is_sequence_container : public std::integral_constant< bool, false> {};

         template< typename T>
         struct is_sequence_container< std::vector< T>> : public std::integral_constant< bool, true> {};

         template< typename T>
         struct is_sequence_container< std::deque< T>> : public std::integral_constant< bool, true> {};

         template< typename T>
         struct is_sequence_container< std::list< T>> : public std::integral_constant< bool, true> {};


         //!
         //! std::vector< char> is the "pod" for binary information
         //!
         template<>
         struct is_sequence_container< std::vector< char>> : public std::integral_constant< bool, false> {};






         template< typename T>
         struct is_associative_set_container : public std::integral_constant< bool, false> {};

         template< typename T>
         struct is_associative_set_container< std::set< T>> : public std::integral_constant< bool, true> {};

         template< typename T>
         struct is_associative_set_container< std::multiset< T>> : public std::integral_constant< bool, true> {};

         template< typename T>
         struct is_associative_set_container< std::unordered_set< T>> : public std::integral_constant< bool, true> {};

         template< typename T>
         struct is_associative_set_container< std::unordered_multiset< T>> : public std::integral_constant< bool, true> {};


         template< typename T>
         struct is_associative_map_container : public std::integral_constant< bool, false> {};

         template< typename K, typename V>
         struct is_associative_map_container< std::map< K, V>> : public std::integral_constant< bool, true> {};

         template< typename K, typename V>
         struct is_associative_map_container< std::multimap< K, V>> : public std::integral_constant< bool, true> {};

         template< typename K, typename V>
         struct is_associative_map_container< std::unordered_map< K, V>> : public std::integral_constant< bool, true> {};

         template< typename K, typename V>
         struct is_associative_map_container< std::unordered_multimap< K, V>> : public std::integral_constant< bool, true> {};


         template< typename T>
         struct is_associative_container : public std::integral_constant< bool,
            is_associative_set_container< T>::value || is_associative_map_container< T>::value> {};



         template< typename T>
         struct is_container_adaptor : public std::integral_constant< bool, false> {};

         template< typename T>
         struct is_container_adaptor< std::stack< T>> : public std::integral_constant< bool, true> {};

         template< typename T>
         struct is_container_adaptor< std::queue< T>> : public std::integral_constant< bool, true> {};

         template< typename T>
         struct is_container_adaptor< std::priority_queue< T>> : public std::integral_constant< bool, true> {};


         template< typename T>
         struct is_container : public std::integral_constant< bool,
            is_sequence_container< T>::value || is_associative_container< T>::value || is_container_adaptor< T>::value > {};


      } // traits
   } // common
} // casual

#endif // TRAITS_H_
