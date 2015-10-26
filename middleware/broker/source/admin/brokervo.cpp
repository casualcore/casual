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
            if( lhs.dependencies.empty() && rhs.dependencies.empty()) { return lhs.id < rhs.id;}

            if( common::range::find( rhs.dependencies, lhs.id))
            {
               if( ! common::range::find( lhs.dependencies, rhs.id))
               {
                  return true;
               }
               else
               {
                  return lhs.id < rhs.id;
               }
            }

            return false;
         }

      } // admin
   } // broker

} // casual
