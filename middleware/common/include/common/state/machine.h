//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/serialize/macro.h"

#include <iosfwd>
#include <type_traits>

namespace casual
{
   namespace common::state
   {
      //! a 'state-machine' - values can only increase
      template< typename Enum, Enum initialize = Enum{}>
      struct Machine
      {
         static_assert( std::is_enum_v< Enum>);

         //! sets the new value if it's greater (in value) than current
         Machine& operator = ( Enum wanted) noexcept
         {
            if( wanted > m_current)
               m_current = wanted;
            return *this;
         }

         auto operator() () const noexcept 
         {
            return m_current;
         }

         inline friend bool operator == ( Machine lhs, Enum rhs) {  return lhs.m_current == rhs;}
         inline friend bool operator == ( Enum lhs, Machine rhs) {  return lhs == rhs.m_current;}
         inline friend bool operator != ( Machine lhs, Enum rhs) {  return lhs.m_current != rhs;}

         inline friend bool operator < ( Machine lhs, Enum rhs) { return lhs.m_current < rhs;}
         inline friend bool operator > ( Machine lhs, Enum rhs) { return lhs.m_current > rhs;}
         inline friend bool operator <= ( Machine lhs, Enum rhs) { return lhs.m_current <= rhs;}
         inline friend bool operator >= ( Machine lhs, Enum rhs) { return lhs.m_current >= rhs;}

         inline friend std::ostream& operator << ( std::ostream& out, Machine value) { return out << description( value.m_current);}

         CASUAL_FORWARD_SERIALIZE( m_current);

      private:
         Enum m_current = initialize;
      };
      
   } // common::state
} // casual