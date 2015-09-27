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

         Transaction::Transaction( ID trid) : trid( std::move( trid)), state( State::active) {}



         Transaction::operator bool() const { return static_cast< bool>( trid);}


         void Transaction::associate( platform::descriptor_type descriptor)
         {
            m_descriptors.push_back( descriptor);
            m_local = false;
         }

         void Transaction::discard( platform::descriptor_type descriptor)
         {
            m_descriptors.erase(
                  std::remove( std::begin( m_descriptors), std::end( m_descriptors), descriptor),
                  std::end( m_descriptors)
            );
         }

         bool Transaction::associated() const
         {
            return ! m_descriptors.empty();
         }

         bool Transaction::associated( platform::descriptor_type descriptor) const
         {
            return static_cast< bool>( range::find( m_descriptors, descriptor));
         }

         const std::vector< platform::descriptor_type>& Transaction::descriptors() const
         {
            return m_descriptors;
         }

         bool Transaction::local() const
         {
            return m_local;
         }

         bool operator == ( const Transaction& lhs, const ID& rhs) { return lhs.trid == rhs;}

         bool operator == ( const Transaction& lhs, const XID& rhs) { return lhs.trid.xid == rhs;}

         std::ostream& operator << ( std::ostream& out, const Transaction& rhs)
         {
            return out << "{trid: " << rhs.trid << ", state: " << rhs.state <<
                  ", timeout: " << rhs.timout <<
                  ", resources: " << range::make( rhs.resources) <<
                  ", descriptors: " << range::make( rhs.m_descriptors) << "}";
         }

      } // transaction
   } // common
} // casual
