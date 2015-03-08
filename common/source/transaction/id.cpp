//!
//! transaction_id.cpp
//!
//! Created on: Nov 1, 2013
//!     Author: Lazan
//!

#include "common/transaction/id.h"
#include "common/uuid.h"
#include "common/process.h"
#include "common/transcode.h"



#include <ios>
#include <sstream>
#include <iomanip>

#include <cstring>




bool operator == ( const XID& lhs, const XID& rhs)
{
   if( lhs.formatID != rhs.formatID) return false;
   if( lhs.formatID == casual::common::transaction::ID::Format::cNull) return true;
   return std::memcmp( &lhs, &rhs, sizeof( XID) - ( XIDDATASIZE - ( lhs.gtrid_length + lhs.bqual_length))) == 0;
}

bool operator < ( const XID& lhs, const XID& rhs)
{
   if( lhs.formatID != rhs.formatID) return lhs.formatID < rhs.formatID;
   if( lhs.formatID == casual::common::transaction::ID::Format::cNull) return false;
   return std::memcmp( &lhs, &rhs, sizeof( XID) - ( XIDDATASIZE - ( lhs.gtrid_length + lhs.bqual_length))) < 0;
}

bool operator != ( const XID& lhs, const XID& rhs)
{
   return ! ( lhs == rhs);
}

std::ostream& operator << ( std::ostream& out, const XID& xid)
{
   if( out && ! casual::common::transaction::null( xid))
   {
      out << casual::common::transcode::hex::encode( xid.data, xid.data + xid.gtrid_length) << ':'
          << casual::common::transcode::hex::encode( xid.data + xid.gtrid_length, xid.data + xid.gtrid_length + xid.bqual_length);
   }
   return out;
}


namespace casual
{
   namespace common
   {
      namespace transaction
      {


         namespace local
         {

            void casualXid( XID& xid, const Uuid& gtrid, const Uuid& bqual)
            {
               xid.formatID = ID::Format::cCasual;

               auto size = sizeof( Uuid::uuid_type);

               memcpy( xid.data, gtrid.get(), size);
               memcpy( xid.data + size, bqual.get(), size);

               xid.gtrid_length = size;
               xid.bqual_length = size;
            }


         } // local


         ID::ID() : ID( process::Handle{})
         {
         }

         ID::ID( process::Handle owner) : m_owner( std::move( owner))
         {
            xid.formatID = Format::cNull;
            xid.gtrid_length = 0;
            xid.bqual_length = 0;
         }

         ID::ID( const XID& xid)
         {
            memcpy( &this->xid, &xid, sizeof( XID));
         }


         ID::ID( const Uuid& gtrid, const Uuid& bqual, process::Handle owner) : m_owner( std::move( owner))
         {
            local::casualXid( xid, gtrid, bqual);
         }


         ID::ID( ID&& rhs) noexcept
         {
            xid = rhs.xid;
            m_owner = std::move( rhs.m_owner);
            rhs.xid.formatID = Format::cNull;

         }
         ID& ID::operator = ( ID&& rhs) noexcept
         {
            xid = rhs.xid;
            m_owner = std::move( rhs.m_owner);
            rhs.xid.formatID = Format::cNull;

            return *this;
         }

         ID::ID( const ID& rhs)
         {
            memcpy( &xid, &rhs.xid, sizeof( XID));
            m_owner = rhs.m_owner;
         }

         ID& ID::operator = ( const ID& rhs)
         {
            memcpy( &xid, &rhs.xid, sizeof( XID));
            m_owner = rhs.m_owner;
            return *this;
         }



         ID ID::create( process::Handle owner)
         {
            return ID( uuid::make(), uuid::make(), std::move( owner));
         }

         ID ID::create()
         {
            return ID( uuid::make(), uuid::make(), process::handle());
         }


         ID ID::branch() const
         {
            ID result( *this);

            if( result)
            {
               auto bqual = uuid::make();

               auto size = sizeof( Uuid::uuid_type);

               memcpy( result.xid.data + result.xid.gtrid_length, bqual.get(), size);

            }

            return result;
         }



         bool ID::null() const
         {
            return xid.formatID == Format::cNull;
         }

         ID::operator bool() const
         {
            return ! transaction::null( xid);
         }



         xid_range_type ID::range() const
         {
            return { xid.data, xid.data + xid.gtrid_length + xid.bqual_length};
         }


         const process::Handle& ID::owner() const
         {
            return m_owner;
         }



         bool operator < ( const ID& lhs, const ID& rhs)
         {
            return lhs.xid < rhs.xid;
         }

         bool operator == ( const ID& lhs, const ID& rhs)
         {
            return lhs.xid == rhs.xid;
         }

         bool operator == ( const ID& lhs, const XID& rhs)
         {
            return lhs.xid == rhs;
         }


         std::ostream& operator << ( std::ostream& out, const ID& id)
         {
            if( out && id)
            {
               out << id.xid << ':' << id.m_owner.pid << ':' << id.m_owner.queue;
            }
            return out;
         }

         xid_range_type global( const XID& xid)
         {
            return { xid.data, xid.data + xid.gtrid_length};
         }

         xid_range_type branch( const XID& xid)
         {
            return { xid.data + xid.gtrid_length,
                  xid.data + xid.gtrid_length + xid.bqual_length};
         }

         bool null( const ID& id)
         {
            return null( id.xid);
         }

         bool null( const XID& id)
         {
            return id.formatID == ID::Format::cNull;
         }

         xid_range_type global( const ID& id)
         {
            return global( id.xid);
         }

         xid_range_type branch( const ID& id)
         {
            return branch( id.xid);
         }


      } // transaction
   } // common
} // casual
