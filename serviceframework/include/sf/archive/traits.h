

#ifndef CASUAL_ARCHIVE_TRAITS_H_
#define CASUAL_ARCHIVE_TRAITS_H_


#include "common/traits.h"

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

         using namespace common::traits;


         template< typename T>
         struct is_pod : public std::integral_constant< bool, std::is_pod< T>::value && ! std::is_class< T>::value && ! std::is_enum< T>::value> {};

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
            ! is_pod< T>::value && ! std::is_enum< T>::value && ! is_container< T>::value> {};


      }


   }


}






#endif
