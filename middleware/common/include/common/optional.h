//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_OPTIONAL_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_OPTIONAL_H_

#include "../../../../thirdparty/stl/optional.hpp"


namespace casual
{
   namespace common
   {
      using std::experimental::optional;

      namespace value
      {
         template< typename T, T empty_value>
         struct optional
         {
            optional() : m_value{ empty_value} {}
            optional( T value) : m_value{ std::move( value)} {}

            explicit operator bool () const { return ! empty();}
            bool empty() const { return m_value == empty_value;}

            operator T () { return m_value;}
            operator const T& () const { return m_value;}

         private:
            T m_value;
         };

      } // value

   } // common
} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_OPTIONAL_H_
