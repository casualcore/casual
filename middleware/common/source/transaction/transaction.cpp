//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/transaction/transaction.h"
#include "common/log/stream.h"
#include "common/algorithm/container.h"
#include "common/log/category.h"

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

         void Transaction::associate( const correlation_type& correlation)
         {
            m_external = true;
            m_pending.push_back( correlation);
         }

         void Transaction::replied( const correlation_type& correlation)
         {
            algorithm::container::trim( m_pending, algorithm::remove( m_pending, correlation));
         }

         void Transaction::involve( strong::resource::id id)
         {
            log::line( log::category::transaction, "involved id: ", id);
            algorithm::append_unique_value( id, m_involved);
         }

         bool Transaction::associate_dynamic( strong::resource::id id)
         {
            return algorithm::append_unique_value( id, m_dynamic); 
         }

         bool Transaction::disassociate_dynamic( strong::resource::id id)
         {
            if( auto found = common::algorithm::find( m_dynamic, id))
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

         bool Transaction::associated( const correlation_type& correlation) const
         {
            return ! algorithm::find( m_pending, correlation).empty();
         }

         const std::vector< Transaction::correlation_type>& Transaction::correlations() const
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

         std::string_view description( Transaction::State value) noexcept
         {
            using Enum = Transaction::State;
            switch( value)
            {
               case Enum::active: return "active";
               case Enum::rollback: return "rollback";
               case Enum::timeout: return "timeout";
            }
            return "<unknown>";
         }


      } // transaction
   } // common
} // casual
