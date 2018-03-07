//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_COMMON_COMPARE_H_
#define CASUAL_COMMON_COMPARE_H_

namespace casual
{
   namespace common 
   {
      template< typename T>
      struct Compare
      {
         constexpr friend bool operator == ( const Compare& lhs, const Compare& rhs) { return lhs.type().tie() == rhs.type().tie();}
         constexpr friend bool operator != ( const Compare& lhs, const Compare& rhs) { return lhs.type().tie() != rhs.type().tie();}
         constexpr friend bool operator < ( const Compare& lhs, const Compare& rhs) { return lhs.type().tie() < rhs.type().tie();}
         constexpr friend bool operator <= ( const Compare& lhs, const Compare& rhs) { return lhs.type().tie() <= rhs.type().tie();}
         constexpr friend bool operator > ( const Compare& lhs, const Compare& rhs) { return lhs.type().tie() > rhs.type().tie();}
         constexpr friend bool operator >= ( const Compare& lhs, const Compare& rhs) { return lhs.type().tie() >= rhs.type().tie();}
      private:
         constexpr const T& type() const { return static_cast< const T&>( *this);} 
      };

   } // common 
} // casual


#endif