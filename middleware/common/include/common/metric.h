//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/platform.h"
#include "common/marshal/marshal.h"

#include "common/stream.h"
#include "common/chronology.h"

#include <ostream>

namespace casual
{
   namespace common
   {
      struct Metric
      {
         struct Limit
         {
            platform::time::unit min = platform::time::limit::zero();
            platform::time::unit max = platform::time::limit::zero();

            template< typename D>
            friend Limit& operator += ( Limit& value, D&& duration)
            {
               auto time = std::chrono::duration_cast< common::platform::time::unit>( duration);
               if( value.min == platform::time::limit::zero())
                  value.min = time;
               else
                  value.min = std::min( value.min, time);
               
               value.max = std::max( value.max, time);
               return value;
            }

            CASUAL_CONST_CORRECT_MARSHAL(
            {
               CASUAL_MARSHAL( min);
               CASUAL_MARSHAL( max);
            })

            inline friend std::ostream& operator << ( std::ostream& out, const Limit& value)
            {
               return stream::write( out, "{ min: ", value.min, ", max: ", value.max, '}');
            }

         };

         platform::size::type count = 0;
         platform::time::unit total = platform::time::limit::zero();
         Limit limit;

         template< typename D>
         friend Metric& operator += ( Metric& value, D&& duration)
         {
            ++value.count;
            value.total += std::chrono::duration_cast< common::platform::time::unit>( duration);
            value.limit += duration;
            return value;
         }


         CASUAL_CONST_CORRECT_MARSHAL(
         {
            CASUAL_MARSHAL( count);
            CASUAL_MARSHAL( total);
            CASUAL_MARSHAL( limit);
         })

         inline friend std::ostream& operator << ( std::ostream& out, const Metric& value)
         {
            return stream::write( out, "{ count: ", value.count, ", total: ", value.total, ", limit: ", value.limit, '}');
         }

      };
   } // common
} // casual