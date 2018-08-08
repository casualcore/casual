//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/transaction/id.h"
#include "common/uuid.h"
#include "common/process.h"
#include "common/transcode.h"
#include "common/memory.h"



#include <ios>
#include <sstream>
#include <iomanip>



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
      casual::common::transcode::hex::encode( out, xid.data, xid.data + xid.gtrid_length);
      out  << ':';
      casual::common::transcode::hex::encode( out, xid.data + xid.gtrid_length, xid.data + xid.gtrid_length + xid.bqual_length);
      out << ':' << xid.formatID;

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
            namespace
            {

               void casual_xid( Uuid gtrid, Uuid bqual, XID& xid )
               {
                  xid.formatID = ID::Format::cCasual;

                  xid.gtrid_length = memory::size( bqual.get());
                  xid.bqual_length = xid.gtrid_length;

                  algorithm::copy( gtrid.get(), xid.data);
                  algorithm::copy( bqual.get(), xid.data + xid.gtrid_length);
               }

            } // <unnamed>
         } // local

         ID::ID() noexcept
         {
            xid.formatID = Format::cNull;
         }



         ID::ID( const process::Handle& owner) : m_owner( std::move( owner))
         {
            xid.formatID = Format::cNull;
         }

         ID::ID( const xid_type& xid) : xid( xid)
         {
         }


         ID::ID( Uuid gtrid, Uuid bqual, const process::Handle& owner) : m_owner( std::move( owner))
         {
            local::casual_xid( gtrid, bqual, xid);
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


         ID ID::create( const process::Handle& owner)
         {
            return ID( uuid::make(), uuid::make(), owner);
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
               auto uuid = uuid::make();

               algorithm::copy( uuid.get(), result.xid.data + result.xid.gtrid_length);
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
            return data( xid);
         }


         const process::Handle& ID::owner() const
         {
            return m_owner;
         }

         void ID::owner( const process::Handle& handle)
         {
            m_owner = handle;
         }



         bool operator < ( const ID& lhs, const ID& rhs)
         {
            return lhs.xid < rhs.xid;
         }

         bool operator == ( const ID& lhs, const ID& rhs)
         {
            return lhs.xid == rhs.xid;
         }

         bool operator == ( const ID& lhs, const xid_type& rhs)
         {
            return lhs.xid == rhs;
         }


         std::ostream& operator << ( std::ostream& out, const ID& id)
         {
            if( out && id)
            {
               out << id.xid << ':' << id.m_owner.pid;
            }
            return out;
         }

         xid_range_type global( const xid_type& xid)
         {
            return xid_range_type{ xid.data, xid.data + xid.gtrid_length};
         }

         xid_range_type branch( const xid_type& xid)
         {
            return xid_range_type{ xid.data + xid.gtrid_length,
                  xid.data + xid.gtrid_length + xid.bqual_length};
         }

         xid_range_type data( const xid_type& xid)
         {
            return xid_range_type{ xid.data, xid.data + xid.gtrid_length + xid.bqual_length};
         }

         bool null( const ID& id)
         {
            return null( id.xid);
         }

         bool null( const xid_type& id)
         {
            return id.formatID == ID::Format::cNull;
         }

         xid_range_type data( const ID& id)
         {
            return data( id.xid);
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
