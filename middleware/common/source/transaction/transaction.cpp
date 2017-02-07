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


         void Transaction::associate( const Uuid& correlation)
         {
            m_pending.push_back( correlation);
            m_local = false;
         }

         void Transaction::replied( const Uuid& correlation)
         {
            m_pending.erase(
                  std::remove( std::begin( m_pending), std::end( m_pending), correlation),
                  std::end( m_pending)
            );
         }

         bool Transaction::pending() const
         {
            return ! m_pending.empty();
         }



         bool Transaction::associated( const Uuid& correlation) const
         {
            return range::find( m_pending, correlation);
         }

         const std::vector< Uuid>& Transaction::correlations() const
         {
            return m_pending;
         }

         bool Transaction::local() const
         {
            return m_local;
         }

         void Transaction::external()
         {
            m_local = false;
         }

         bool operator == ( const Transaction& lhs, const ID& rhs) { return lhs.trid == rhs;}

         bool operator == ( const Transaction& lhs, const XID& rhs) { return lhs.trid.xid == rhs;}

         std::ostream& operator << ( std::ostream& out, const Transaction& rhs)
         {
            return out << "{trid: " << rhs.trid << ", state: " << rhs.state <<
                  ", timeout: " << rhs.timout <<
                  ", resources: " << range::make( rhs.resources) <<
                  ", pending: " << range::make( rhs.m_pending) << "}";
         }

      } // transaction
   } // common
} // casual
