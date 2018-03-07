//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_COMMON_MARSHAL_MARSHAL_H_
#define CASUAL_COMMON_MARSHAL_MARSHAL_H_

#include <array>


#define CASUAL_CONST_CORRECT_MARSHAL( statement) \
   template< typename A> \
   void marshal( A& archive)  \
   {  \
         statement \
   } \
   template< typename A> \
   void marshal( A& archive) const \
   {  \
         statement \
   } \

namespace casual
{
   template< typename T, typename M>
   void casual_marshal( T& value, M& marshler)
   {
      value.marshal( marshler);
   }

   template< typename T, typename M>
   void casual_marshal_value( T& value, M& marshler)
   {
      using casual::casual_marshal;
      casual_marshal( value, marshler);
   }

   template< typename T, typename M>
   void casual_unmarshal_value( T& value, M& unmarshler)
   {
      using casual::casual_marshal;
      casual_marshal( value, unmarshler);
   }

} // casual

#endif // MARSHAL_H_
