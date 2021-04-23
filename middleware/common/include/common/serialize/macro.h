//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/serialize/named/value.h"
#include "common/serialize/traits.h"

namespace casual
{
   namespace common::serialize::detail
   {
      template< typename A, typename V> 
      auto dispatch( A& archive, V&& value, const char* name)
      {
         if constexpr( traits::need::named_v< A>)
            archive & named::value::make( std::forward< V>( value), name);
         else
            archive & std::forward< V>( value);
      }
 
   } // common::serialize::detail
} // casual

#define CASUAL_CONST_CORRECT_SERIALIZE( statement) \
   template< typename A>  \
   void serialize( A& archive) \
   {  \
      statement  \
   } \
   template< typename A>  \
   void serialize( A& archive) const\
   {  \
      statement  \
   } \

// to forward one value
#define CASUAL_FORWARD_SERIALIZE( member) \
   template< typename A>  \
   void serialize( A& archive, const char* name) \
   {  \
      casual::common::serialize::detail::dispatch( archive, member, name); \
   } \
   template< typename A>  \
   void serialize( A& archive, const char* name) const \
   {  \
      casual::common::serialize::detail::dispatch( archive, member, name); \
   } \


#define CASUAL_LOG_SERIALIZE( statement) \
   template< typename A>  \
   void serialize( A& archive) const\
   {  \
      statement  \
   } \

#define CASUAL_SERIALIZE( value) \
   casual::common::serialize::detail::dispatch( archive, value, #value)

#define CASUAL_SERIALIZE_NAME( value, name) \
   casual::common::serialize::detail::dispatch( archive, value, name)

#define CASUAL_NAMED_VALUE( member) \
   casual::common::serialize::named::value::make( member, #member)

#define CASUAL_NAMED_VALUE_NAME( member, name) \
   casual::common::serialize::named::value::make( member, name)




