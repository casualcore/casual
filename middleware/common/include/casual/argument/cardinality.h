//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/code/raise.h"
#include "common/code/casual.h"
#include "common/serialize/macro.h"

#include <limits>
#include <type_traits>

namespace casual
{
   namespace argument
   {
      namespace cardinality
      {
          using value_type = long;
         
      } // cardinality

      struct Cardinality
      {
         using value_type = cardinality::value_type;

         constexpr Cardinality( value_type min, value_type max, value_type step) 
            : m_min( min), m_max( max), m_step( step) 
         {
            if( min < 0 || max < 0 || min > max || step < 0 || step > max)
               common::code::raise::error( common::code::casual::invalid_argument, "invalid cardinality: [", min, ',', max, ',', step, "]");
         }

         constexpr Cardinality( value_type min, value_type max) : Cardinality{ min, max, 0} {} 

         constexpr Cardinality() : Cardinality{ 0, 1} {}

         //! @return true if `value` is within the cardinality
         constexpr bool valid( value_type value) const 
         {
            if( m_step == 0) 
               return value >= m_min && value <= m_max;
            else  
               return value >= m_min && value <= m_max && ( value - m_min ) % m_step == 0;
         }

         constexpr auto min() const { return m_min;}
         constexpr auto max() const { return m_max;}
         constexpr auto step() const { return m_step;}

         constexpr bool fixed() const { return m_min == m_max;}
         constexpr bool many() const { return m_max == std::numeric_limits< value_type>::max();}

         //! @returns a cardinality with same min max and the provided `step`.
         constexpr Cardinality steps( value_type step) { return Cardinality{ min(), max(), step};}

         constexpr friend Cardinality operator + ( Cardinality lhs, Cardinality rhs)
         { 
            lhs.m_min += rhs.m_min;
            lhs.m_max = lhs.many() ? lhs.m_max : ( rhs.many() ? rhs.m_max : ( lhs.m_max + rhs.m_max));
            return lhs;
         }

         constexpr friend bool operator == ( const Cardinality& lhs, const Cardinality& rhs) = default;

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( m_min);
            CASUAL_SERIALIZE( m_max);
            CASUAL_SERIALIZE( m_step);
         )

      private:
         value_type m_min;
         value_type m_max;
         value_type m_step;
      };

      namespace cardinality
      {
         constexpr auto range( value_type min, value_type max, value_type step) { return Cardinality{ min, max, step};}
         constexpr auto range( value_type min, value_type max) { return Cardinality{ min, max};}
         constexpr auto fixed( value_type value) { return range( value, value);}
         constexpr auto min( value_type value) { return Cardinality{ value, std::numeric_limits< value_type>::max()};}
         constexpr auto max( value_type value) { return Cardinality{ 0, value};}
         constexpr auto one() { return fixed( 1);}
         constexpr auto zero() { return fixed( 0);}
         constexpr auto zero_one() { return max( 1);}
         constexpr auto any() { return min( 0);}
         constexpr auto one_many() { return min( 1);}
         
      } // cardinality


   } // argument
} // casual

