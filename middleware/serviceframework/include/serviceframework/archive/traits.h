//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#pragma once



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
   namespace serviceframework
   {
      namespace traits
      {

         using namespace common::traits;


         template< typename T>
         struct is_pod : public traits::bool_constant< std::is_pod< T>::value && ! std::is_class< T>::value && ! std::is_enum< T>::value> {};

         template<>
         struct is_pod< std::string> : public traits::bool_constant< true> {};

         template<>
         struct is_pod< std::wstring> : public traits::bool_constant< true> {};

         template<>
         struct is_pod< std::vector< char> > : public traits::bool_constant< true> {};


         template< typename T, typename... Args>
         struct has_serialize
         {
         private:
            using one = char;
            struct two { char m[ 2];};

            template< typename C, typename... A> static auto test( C&& c, A&&... a) -> decltype( (void)( c.serialize( a...)), one());
            static two test(...);
         public:
            enum { value = sizeof( test( std::declval< T>(), std::declval< Args>()...)) == sizeof(one) };
         };


      } // traits
   } // serviceframework
} // casual

