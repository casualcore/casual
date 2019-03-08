//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/outbound/route.h"

#include "common/chronology.h"

namespace casual
{
   namespace gateway
   {
      namespace outbound
      {
         namespace route
         {
            std::ostream& operator << ( std::ostream& out, const Point& value)
            {
               return out << "{ correlation: " << value.correlation
                     << ", destination: " << value.destination
                     << ", type: "<< value.type
                     << '}';
            }

            namespace service
            {
               std::ostream& operator << ( std::ostream& out, const Point& value)
               {
                  return out << "{ correlation: " << value.correlation
                        << ", destination: " << value.destination
                        << ", service: "<< value.service
                        << ", parent: "<< value.parent
                        << ", start: "<< common::chronology::local( value.start) << "us"
                        << '}';
               }

            } // service
         } // route
      } // outbound
   } // gateway
} // casual