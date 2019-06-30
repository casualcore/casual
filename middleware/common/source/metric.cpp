//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/metric.h"

#include "common/stream.h"
#include "common/chronology.h"

namespace casual
{
   namespace common
   {
      namespace local
      {
         namespace
         {
            auto min( platform::time::unit lhs, platform::time::unit rhs)
            {
               if( lhs == platform::time::limit::zero())
                  return rhs;
               
               return std::min( lhs, rhs);
            }
         } // <unnamed>
      } // local

      Metric::Limit& Metric::Limit::operator += ( platform::time::unit duration)
      {
         min = local::min( min, duration);
         max = std::max( max, duration);
         return *this;
      }

      Metric::Limit& Metric::Limit::operator += ( const Metric::Limit& rhs)
      {
         min = local::min( min, rhs.min);
         max = std::max( max, rhs.max);
         return *this;
      }

      Metric::Limit operator + ( const Metric::Limit& lhs, const Metric::Limit& rhs)
      {
         return {
            local::min( lhs.min, rhs.min), 
            std::max( lhs.max, rhs.max)
         };
      }

      Metric& Metric::operator += ( common::platform::time::unit duration)
      {
         ++count;
         total += duration;
         limit += duration;
         return *this;
      }

      Metric& Metric::operator += ( const Metric& rhs)
      {
         count += rhs.count;
         total += rhs.total;
         limit += rhs.limit;
         return *this;
      }

      Metric operator + ( const Metric& lhs, const Metric& rhs)
      {
         return {
            lhs.count + rhs.count,
            lhs.total + rhs.total,
            lhs.limit + rhs.limit
         };
      }

      bool operator == ( const Metric& lhs, const Metric& rhs)
      {
         auto tie = []( auto& v){ return std::tie( v.count, v.total, v.limit.min, v.limit.max);};
         return tie( lhs) == tie( rhs);
      }

   } // common
} // casual