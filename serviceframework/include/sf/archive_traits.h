

#ifndef CASUAL_ARCHIVE_TRAITS_H_
#define CASUAL_ARCHIVE_TRAITS_H_

#include <type_traits>


#include <string>

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
   namespace sf
   {
      namespace traits
      {
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



         template< typename T>
         struct is_pod : public std::integral_constant< bool, std::is_pod< T>::value && ! std::is_class< T>::value> {};

         template<>
         struct is_pod< std::string> : public std::integral_constant< bool, true> {};

         template<>
         struct is_pod< std::wstring> : public std::integral_constant< bool, true> {};

         template<>
         struct is_pod< std::vector< char> > : public std::integral_constant< bool, true> {};


         // TODO: We want to only say T is serializible if T has member function void serialize( Archive& a) (const)
         //
         template< typename T>
         struct is_serializible : public std::integral_constant< bool,
            ! is_pod< T>::value && ! is_container< T>::value> {};


      }


   }


}






#endif
