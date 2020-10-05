//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "queue/forward/state.h"

#include <ostream>

namespace casual
{
   namespace queue
   {
      namespace forward
      {

         namespace state
         {

            std::ostream& operator << ( std::ostream& out, Machine value)
            {
               switch( value)
               {
                  case Machine::startup: return out << "startup";
                  case Machine::running: return out << "running";
                  case Machine::shutdown: return out << "shutdown";
               }
               return out << "<unknown>";
            }

         } // state


      } // forward
   } // queue
} // casual