//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

namespace casual
{
   namespace common
   {
      namespace stream
      {
         //! to enable customization points for stream/logging
         namespace customization
         {
            //! kicks in if T does not have an ostream stream operator
            template< typename T, typename Enable = void>
            struct point;

            namespace supersede
            {
               // highest priority, and will supersede ostream stream operator
               // used to 'override' standard defined stream operators
               template< typename T, typename Enable = void>
               struct point;
            } // supersede
                        
         } // customization
      } // stream
   } // common
} // casual