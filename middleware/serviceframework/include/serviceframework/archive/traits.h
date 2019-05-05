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
         struct is_pod : public traits::bool_constant< 
            ! std::is_enum< T>::value
            && ( ( std::is_pod< T>::value && ! std::is_class< T>::value)
               || std::is_convertible< T&, std::string&>::value
               || std::is_convertible< const T&, const std::string&>::value
               || std::is_same< traits::remove_cvref_t< T>, common::platform::binary::type>::value
            )
            > {};

      } // traits
   } // serviceframework
} // casual

