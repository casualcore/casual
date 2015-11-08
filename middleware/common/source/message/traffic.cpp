/*
 * monitor.cpp
 *
 *  Created on: 22 apr 2015
 *      Author: 40043280
 */

#include "common/message/traffic.h"

namespace casual
{
   namespace common
   {

      namespace message
      {
         namespace traffic
         {

            std::ostream& operator << ( std::ostream& out, const Event& value)
            {
               return out << "{ service: " << value.service << ", parent: " << value.parent
                  << ", start: " << std::chrono::duration_cast< std::chrono::milliseconds>( value.start.time_since_epoch()).count()
                  << ", end: " << std::chrono::duration_cast< std::chrono::milliseconds>( value.end.time_since_epoch()).count() << '}';
            }

         } // traffic
      } // message
   } // common
} // casual
