//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#ifndef CASUAL_COMMON_ARGUMENT_CARDINALITY_H_
#define CASUAL_COMMON_ARGUMENT_CARDINALITY_H_

#include "common/platform.h"
#include "common/compare.h"


#include <iosfwd>

namespace casual
{
   namespace common
   {
      namespace argument
      {
         namespace cardinality 
         {
            using value_type = platform::size::type;

            template< value_type min_value, value_type max_value, value_type step_value = 0>
            struct range
            {

               enum Value : value_type 
               {
                  min = min_value,
                  max = max_value,
                  step = step_value
               };

               static_assert( min >= 0 && max >= 0 && min <= max && step >= 0 && step <= max, "not a valid cardinality");

               constexpr static bool fixed() { return min == max;}
               constexpr static auto many() { return max == std::numeric_limits< value_type>::max();}

               template< value_type step_v>
               constexpr static auto steps() { return range< min, max, step_v>{};}

               template< value_type r_min, value_type r_max, value_type r_step>
               constexpr friend auto operator + ( range lhs, range< r_min, r_max, r_step> rhs)
               {
                  constexpr auto max = lhs.many() ? lhs.max : ( rhs.many() ? r_max : ( lhs.max + r_max));
                  return range< lhs.min + r_min, max, rhs.step>{};
               }

#ifndef __clang__
// g++ 5.4 bug               
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-parameter"
#endif

               template< value_type r_min, value_type r_max, value_type r_step>
               constexpr friend auto operator == ( range lhs, range< r_min, r_max, r_step> rhs)
               {
                  return lhs.min == r_min &&  lhs.max == r_max;
               }

#ifndef __clang__
#pragma GCC diagnostic pop
#endif

            };

            template< value_type size>
            using fixed = range< size, size>;

            template< value_type size>
            using min = range< size, std::numeric_limits< value_type>::max()>;

            template< value_type size>
            using max = range< 0, size>;

            using one = fixed< 1>;
            using zero = fixed< 0>;
            using zero_one = max< 1>;
            using any = min< 0>;
            using one_many = min< 1>;



         } // cardinality 

         struct Cardinality : compare::Equality< Cardinality>
         {
            using value_type = platform::size::type;

            template< value_type min, value_type max, value_type step>
            constexpr Cardinality( cardinality::range< min, max, step>) : m_min( min), m_max( max), m_step( step) {}
            constexpr Cardinality() : Cardinality{ cardinality::zero_one()} {}

            constexpr bool valid( value_type value) const 
            {
               if( m_step == 0) 
                  return value >= m_min && value <= m_max;
               else  
                  return value >= m_min && value <= m_max && ( value - m_min ) % m_step == 0;
            }

            friend auto tie( const Cardinality& value) { return std::tie( value.m_min, value.m_max);}

            constexpr auto min() const { return m_min;}
            constexpr auto max() const { return m_max;}
            constexpr auto step() const { return m_step;}

            constexpr auto fixed() const { return min() == max();}
            constexpr auto many() const { return max() == std::numeric_limits< value_type>::max();}

            constexpr friend Cardinality operator + ( Cardinality lhs, Cardinality rhs)
            { 
               lhs.m_min += rhs.m_min;
               lhs.m_max = lhs.many() ? lhs.max() : ( rhs.many() ? rhs.max() : ( lhs.max() + rhs.max()));
               
               return lhs;
            }

            friend std::ostream& operator << ( std::ostream& out, Cardinality value);

         private:
            value_type m_min;
            value_type m_max;
            value_type m_step;
         };
      } // argument
   } // common
} // casual



#endif
