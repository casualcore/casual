//!
//! casual
//!

#ifndef CASUAL_COMMON_SAFE_HANDLE_H_
#define CASUAL_COMMON_SAFE_HANDLE_H_

#include "common/marshal/marshal.h"

#include <iosfwd>

namespace casual
{
   namespace common
   {
      namespace safe 
      {
         template< typename Integer, typename Tag, Integer initial = -1>
         class basic_handle
         { 
         public:

            using native_type = Integer;

            constexpr basic_handle() = default;
            constexpr inline explicit basic_handle( native_type id) : m_id( id) {}             

            constexpr inline native_type native() const { return m_id;}

            constexpr inline bool valid() const { return m_id != initial;}

            constexpr inline explicit operator bool () const { return valid();}

            constexpr inline friend bool operator == ( basic_handle lhs, basic_handle rhs) { return lhs.m_id == rhs.m_id; }
            constexpr inline friend bool operator != ( basic_handle lhs, basic_handle rhs) { return ! ( lhs == rhs); }
            constexpr inline friend bool operator < ( basic_handle lhs, basic_handle rhs) { return lhs.m_id < rhs.m_id; }

            constexpr inline friend bool operator == ( basic_handle lhs, native_type rhs) { return lhs.m_id == rhs; }

            inline friend std::ostream& operator << ( std::ostream& out, basic_handle value) { return out << value.m_id;}

            CASUAL_CONST_CORRECT_MARSHAL(
            {
               archive & m_id;
            })

         private:
            native_type m_id = initial;
         };

      } // safe 
   } // common
} // casual

namespace std 
{
   template< typename Integer, typename Tag, Integer initial> 
   struct hash< casual::common::safe::basic_handle< Integer, Tag, initial>>
   {
     auto operator()( const casual::common::safe::basic_handle< Integer, Tag, initial>& value) const
     {
       return std::hash< decltype( value.native())>{}( value.native());
     }
   };
}


#endif
