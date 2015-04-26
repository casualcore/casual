/*
 * monitor.cpp
 *
 *  Created on: 22 apr 2015
 *      Author: 40043280
 */

#include "common/message/monitor.h"

namespace casual
{
   namespace common
   {

      namespace message
      {
         namespace traffic_monitor
         {

            std::ostream& operator << ( std::ostream& out, const Notify& value)
            {
               return out << "{ service: " << value.service << ", parent: " << value.parentService
                  << ", start: " << std::chrono::duration_cast< std::chrono::milliseconds>( value.start.time_since_epoch()).count()
                  << ", end: " << std::chrono::duration_cast< std::chrono::milliseconds>( value.end.time_since_epoch()).count() << '}';
            }


         } // monitor

      } // message
   } // common
} // casual
