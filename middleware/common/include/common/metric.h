//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/platform.h"
#include "common/serialize/macro.h"
#include "common/chronology.h"


#include <iosfwd>

namespace casual
{
   namespace common
   {
      struct Metric
      {
         struct Limit
         {
            chronology::duration min = chronology::duration::zero();
            chronology::duration max = chronology::duration::zero();

            Limit& operator += ( chronology::duration duration);
            Limit& operator += ( const Limit& rhs);

            friend Limit operator + ( const Limit& lhs, const Limit& rhs);
            
            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               CASUAL_SERIALIZE( min);
               CASUAL_SERIALIZE( max);
            })
         };

         platform::size::type count = 0;
         chronology::duration total = chronology::duration::zero();
         Limit limit;


         Metric& operator += ( chronology::duration duration);
         Metric& operator += ( const Metric& rhs);
         friend Metric operator + ( const Metric& lhs, const Metric& rhs);

         template< typename R, typename P>
         Metric& operator += ( std::chrono::duration< R, P> duration)
         {
            return *this += std::chrono::duration_cast< chronology::duration>( duration);
         }

         friend bool operator == ( const Metric& lhs, const Metric& rhs);
         inline friend bool operator != ( const Metric& lhs, const Metric& rhs) { return ! ( lhs == rhs);}


         CASUAL_CONST_CORRECT_SERIALIZE(
         {
            CASUAL_SERIALIZE( count);
            CASUAL_SERIALIZE( total);
            CASUAL_SERIALIZE( limit);
         })
      };
   } // common
} // casual