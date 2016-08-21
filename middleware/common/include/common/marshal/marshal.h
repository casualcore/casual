//!
//! casual
//!

#ifndef CASUAL_COMMON_MARSHAL_MARSHAL_H_
#define CASUAL_COMMON_MARSHAL_MARSHAL_H_


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
   void casual_marshal_value( T& value, M& marshler)
   {
      value.marshal( marshler);
   }

   template< typename T, typename M>
   void casual_unmarshal_value( T& value, M& unmarshler)
   {
      value.marshal( unmarshler);
   }

} // casual

#endif // MARSHAL_H_
