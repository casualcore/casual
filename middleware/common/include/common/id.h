//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_ID_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_ID_H_

#include "common/marshal/marshal.h"
#include "common/platform.h"

namespace casual
{
   namespace common
   {
      namespace id
      {
         namespace tag
         {
            struct standard{};

         } // tag

         //!
         //! "id" abstraction to help make "id-handling" more explicit and
         //! enable overloading on a specific id-type
         //!
         template< typename I, typename Tag = id::tag::standard, I start = 10>
         struct basic
         {
            using id_type = I;

            basic() = default;
            explicit basic( id_type id) : m_id( id) {}


            explicit operator id_type() const { return m_id;}

            id_type underlaying() const { return m_id;}

            basic operator ++ () { ++m_id; return *this;}
            basic operator ++ ( int) { auto result = *this; ++m_id; return result;}

            friend bool operator < ( basic lhs, basic rhs) { return lhs.m_id < rhs.m_id;}
            friend bool operator == ( basic lhs, basic rhs) { return lhs.m_id == rhs.m_id;}
            friend bool operator != ( basic lhs, basic rhs) { return ! ( lhs == rhs);}
            friend basic operator + ( basic lhs, basic rhs) { return basic{ lhs.m_id + rhs.m_id};}


            static basic next()
            {
               return counter()++;
            }

            static void next( basic value)
            {
               counter() = value;
            }

            friend std::ostream& operator << ( std::ostream& out, basic value) { return out << value.m_id;}

            CASUAL_CONST_CORRECT_MARSHAL({
               archive & m_id;
            })

         private:

            static basic& counter()
            {
               static basic value{ start};
               return value;
            }

            id_type m_id = id_type();
         };

         template< typename I, typename Tag, I start>
         auto underlaying( basic< I, Tag, start> value)
         {
            return value.underlaying();
         }

      } // id

      using Id = id::basic< platform::size::type>;

   } // common
} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_ID_H_
