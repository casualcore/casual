//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/platform.h"
#include "common/marshal/marshal.h"


#include <iosfwd>

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

            Limit& operator += ( platform::time::unit duration);
            Limit& operator += ( const Limit& rhs);

            friend Limit operator + ( const Limit& lhs, const Limit& rhs);
            

            CASUAL_CONST_CORRECT_MARSHAL(
            {
               CASUAL_MARSHAL( min);
               CASUAL_MARSHAL( max);
            })

            friend std::ostream& operator << ( std::ostream& out, const Limit& value);

         };

         platform::size::type count = 0;
         platform::time::unit total = platform::time::limit::zero();
         Limit limit;


         Metric& operator += ( platform::time::unit duration);
         Metric& operator += ( const Metric& rhs);
         friend Metric operator + ( const Metric& lhs, const Metric& rhs);

         template< typename R, typename P>
         Metric& operator += ( std::chrono::duration< R, P> duration)
         {
            return *this += std::chrono::duration_cast< platform::time::unit>( duration);
         }

         friend bool operator == ( const Metric& lhs, const Metric& rhs);
         inline friend bool operator != ( const Metric& lhs, const Metric& rhs) { return ! ( lhs == rhs);}


         CASUAL_CONST_CORRECT_MARSHAL(
         {
            CASUAL_MARSHAL( count);
            CASUAL_MARSHAL( total);
            CASUAL_MARSHAL( limit);
         })

         friend std::ostream& operator << ( std::ostream& out, const Metric& value);

      };
   } // common
} // casual