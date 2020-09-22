//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/transaction/transaction.h"
#include "common/log/stream.h"

#include <ostream>

namespace casual
{
   namespace common
   {
      namespace transaction
      {

         Transaction::Transaction() = default;
         Transaction::Transaction( ID trid) : trid( std::move( trid)) {}

         Transaction::operator bool() const { return static_cast< bool>( trid);}


         void Transaction::associate( const Uuid& correlation)
         {
            m_external = true;
            m_pending.push_back( correlation);
         }

         void Transaction::replied( const Uuid& correlation)
         {
            algorithm::trim( m_pending, algorithm::remove( m_pending, correlation));
         }

         void Transaction::involve( strong::resource::id id)
         {
            algorithm::push_back_unique( id, m_involved);
         }

         bool Transaction::associate_dynamic( strong::resource::id id)
         {
            return algorithm::push_back_unique( id, m_dynamic); 
         }

         bool Transaction::disassociate_dynamic( strong::resource::id id)
         {
            auto found = common::algorithm::find( m_dynamic, id);

            if( found)
            {
               m_dynamic.erase( std::begin( found));
               return true;
            }
            return false;
         }

         bool Transaction::pending() const
         {
            return ! m_pending.empty();
         }

         bool Transaction::associated( const Uuid& correlation) const
         {
            return ! algorithm::find( m_pending, correlation).empty();
         }

         const std::vector< Uuid>& Transaction::correlations() const
         {
            return m_pending;
         }

         bool Transaction::local() const
         {
            return trid.owner().pid == process::id() && ! m_external;
         }

         void Transaction::external()
         {
            m_external = true;
         }

         void Transaction::suspend() { m_suspended = true;}
         void Transaction::resume() { m_suspended = false;}
         bool Transaction::suspended() const { return m_suspended;}

         bool operator == ( const Transaction& lhs, const ID& rhs) { return lhs.trid == rhs;}

         bool operator == ( const Transaction& lhs, const XID& rhs) { return lhs.trid.xid == rhs;}


         std::ostream& operator << ( std::ostream& out, Transaction::State value)
         {
            using Enum = Transaction::State;
            switch( value)
            {
               case Enum::active: return out << "active";
               case Enum::rollback: return out << "rollback";
               case Enum::timeout: return out << "timeout";
            }
            return out << "<unknown>";
         }


      } // transaction
   } // common
} // casual
