//!
//! transaction.cpp
//!
//! Created on: Feb 14, 2015
//!     Author: Lazan
//!

#include "common/transaction/transaction.h"

#include <ostream>

namespace casual
{
   namespace common
   {
      namespace transaction
      {

         Transaction::Transaction() = default;

         Transaction::Transaction( ID trid) : trid( std::move( trid)), state( State::active), suspended( false) {}



         Transaction::operator bool() const { return trid && ! suspended;}

         void Transaction::discard( platform::descriptor_type descriptor)
         {
            descriptors.erase(
                  std::remove( std::begin( descriptors), std::end( descriptors), descriptor),
                  std::end( descriptors)
            );
         }

         bool operator == ( const Transaction& lhs, const ID& rhs) { return lhs.trid == rhs;}

         bool operator == ( const Transaction& lhs, const XID& rhs) { return lhs.trid.xid == rhs;}

         std::ostream& operator << ( std::ostream& out, const Transaction& rhs)
         {
            return out << "{trid: " << rhs.trid << ", state: " << rhs.state <<
                  ", suspended: " << std::boolalpha << rhs.suspended <<
                  ", resources: " << range::make( rhs.resources) <<
                  ", descriptors: " << range::make( rhs.descriptors) << "}";
         }

      } // transaction
   } // common
} // casual
