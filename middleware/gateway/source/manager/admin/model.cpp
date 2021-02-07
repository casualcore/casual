//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/manager/admin/model.h"

namespace casual
{
   namespace gateway::manager::admin::model
   {
      inline namespace v2
      {
         namespace connection
         {

            std::ostream& operator << ( std::ostream& out, Phase value)
            {
               switch( value)
               {
                  case Phase::regular: return out << "regular";
                  case Phase::reversed: return out << "reversed";
               }
               return out << "<unknown>";
            }

            std::ostream& operator << ( std::ostream& out, Bound value)
            {
               switch( value)
               {
                  case Bound::out: return out << "out";
                  case Bound::in: return out << "in";
                  case Bound::unknown: return out << "unknown";
               }
               return out << "<unknown>";
            }

            std::ostream& operator << ( std::ostream& out, Runlevel value) { return out << "<not used>";}

         } // connection

        
         namespace group
         {
            std::ostream& operator << ( std::ostream& out, Runlevel value)
            {
               switch( value)
               {
                  case Runlevel::running: return out << "running";
                  case Runlevel::shutdown: return out << "shutdown";
                  case Runlevel::error: return out << "error";
               }
               return out << "<unknown>";
            }
         } // group
      } // v2
      
   } // gateway::manager::admin::model
} // casual