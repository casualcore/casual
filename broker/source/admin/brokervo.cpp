//!
//! brokervo.cpp
//!
//! Created on: Jul 17, 2015
//!     Author: Lazan
//!

#include "broker/admin/brokervo.h"


namespace casual
{
   namespace broker
   {
      namespace admin
      {


         bool operator < ( const GroupVO& lhs, const GroupVO& rhs)
         {
            if( ! lhs.dependencies.empty() && rhs.dependencies.empty()) { return true;}

            if( common::range::find( rhs.dependencies, lhs.id)
               && ! common::range::find( lhs.dependencies, rhs.id)) { return true;}

            return false;
         }

      } // admin
   } // broker

} // casual
