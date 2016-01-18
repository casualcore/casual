//!
//! message.cpp
//!
//! Created on: Jan 6, 2016
//!     Author: Lazan
//!


#include "common/communication/message.h"

namespace casual
{

   namespace common
   {

      namespace communication
      {

         namespace message
         {

            Complete::Complete( Complete&& rhs) noexcept
            {
               swap( *this, rhs);
            }
            Complete& Complete::operator = ( Complete&& rhs) noexcept
            {
               Complete temp{ std::move( rhs)};
               swap( *this, temp);
               return *this;
            }

            Complete::operator bool() const
            {
               return type != message_type_type::absent_message;
            }

            bool Complete::complete() const { return m_unhandled.empty();}

            void swap( Complete& lhs, Complete& rhs)
            {
               using std::swap;
               swap( lhs.correlation, rhs.correlation);
               swap( lhs.payload, rhs.payload);
               swap( lhs.type, rhs.type);
               swap( lhs.m_unhandled, rhs.m_unhandled);
            }

            std::ostream& operator << ( std::ostream& out, const Complete& value)
            {
               return out << "{ type: " << value.type << ", correlation: " << value.correlation << ", size: "
                     << value.payload.size() << std::boolalpha << ", complete: " << value.complete() << '}';
            }

         } // message

      } // communication

   } // common


} // casual
