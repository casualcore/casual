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
#include <iostream>

#include <cassert>


bool operator == ( const XID& lhs, const XID& rhs)
{
   if( lhs.formatID != rhs.formatID) return false;
   if( lhs.formatID == casual::common::transaction::ID::Format::null) return true;
   return std::memcmp( &lhs, &rhs, sizeof( XID) - ( XIDDATASIZE - ( lhs.gtrid_length + lhs.bqual_length))) == 0;
}

bool operator < ( const XID& lhs, const XID& rhs)
{
   if( lhs.formatID != rhs.formatID) return lhs.formatID < rhs.formatID;
   if( lhs.formatID == casual::common::transaction::ID::Format::null) return false;
   return std::memcmp( &lhs, &rhs, sizeof( XID) - ( XIDDATASIZE - ( lhs.gtrid_length + lhs.bqual_length))) < 0;
}

std::ostream& operator << ( std::ostream& out, const XID& xid)
{
   if( ! casual::common::transaction::xid::null( xid))
   {
      // TODO: hack to get rid of concurrent xid read/write in a
      // unittest environment - We should get rid of threads in unittest also...
      if( xid.gtrid_length > 64 || xid.bqual_length > 64)
      {
         // we can't really log to casual.log since this is probably what triggers this...
         // ...although, the write should be done "soon" and the XID should be in a consistent state
         // we'll logg to std::cerr so we at least get some indication still..
         std::cerr << "XID is in an inconsistent state - discard output - XID: { xid.formatID: " << xid.formatID
            << "xid.gtrid_length: " << xid.gtrid_length
            << ", xid.bqual_length: " << xid.bqual_length
            << "}\n";

         return out;
      }

      casual::common::transcode::hex::encode( out, casual::common::transaction::id::range::global( xid));
      out  << ':';
      casual::common::transcode::hex::encode( out, casual::common::transaction::id::range::branch( xid));
      out << ':' << xid.formatID;

   }
   return out;
}


namespace casual
{
   namespace common::transaction
   {

      namespace xid
      {
         XID null() noexcept
         {
            XID xid{};
            xid.formatID = ID::Format::null;
            return xid;
         }

         bool null( const XID& id) noexcept
         {
            return id.formatID == ID::Format::null;
         }
      } // xid

      namespace local
      {
         namespace
         {
            
            template< typename T, typename U>
            void casual_xid( const T& gtrid, const U& bqual, XID& xid )
            {
               xid.gtrid_length = std::size( gtrid);
               xid.bqual_length = std::size( bqual);

               algorithm::copy( view::binary::to_string_like( gtrid), xid.data);
               algorithm::copy( view::binary::to_string_like( bqual), xid.data + xid.gtrid_length);

               xid.formatID = ID::Format::casual;
            }

         } // <unnamed>
      } // local

      ID::ID( const process::Handle& owner) : m_owner( std::move( owner))
      {}

      ID::ID( const xid_type& xid) : xid( xid)
      {}

      ID::ID( global::id::range gtrid)
      {
         auto bqual = uuid::make();
         local::casual_xid( gtrid, bqual.range(), xid);
      }

      ID::ID( Uuid gtrid, Uuid bqual, const process::Handle& owner) : m_owner( std::move( owner))
      {
         local::casual_xid( gtrid.range(), bqual.range(), xid);
      }

      ID::ID( ID&& rhs) noexcept
      {
         xid = std::exchange( rhs.xid, xid);
         m_owner = std::exchange( rhs.m_owner, {});

      }
      ID& ID::operator = ( ID&& rhs) noexcept
      {
         xid = std::exchange( rhs.xid, xid::null());
         m_owner = std::exchange( rhs.m_owner, {});

         return *this;
      }

      bool ID::null() const
      {
         return xid.formatID == Format::null;
      }

      ID::operator bool() const
      {
         return ! xid::null( xid);
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

      bool operator == ( const ID& lhs, global::id::range rhs)
      {
         return algorithm::equal( id::range::global( lhs), rhs);
      }

      std::ostream& operator << ( std::ostream& out, const ID& id)
      {
         if( out && id)
            out << id.xid << ':' << id.m_owner.pid;
         
         return out;
      }

      namespace id
      {
         ID create( const process::Handle& owner)
         {
            return { uuid::make(), uuid::make(), owner};
         }

         ID create()
         {
            return { uuid::make(), uuid::make(), process::handle()};
         }

         bool null( const ID& id)
         {
            return xid::null( id.xid);
         }

         ID branch( const ID& id)
         {
            if( id.null())
               return {};

            ID result( id);

            result.xid.formatID = ID::Format::branch;

            auto uuid = uuid::make();
            auto range = uuid::range( uuid);

            algorithm::copy( range, result.xid.data + result.xid.gtrid_length);
            result.xid.bqual_length = range.size();
            
            return result;
         }

         namespace range
         {
            type::global global( const xid_type& xid)
            {
               return type::global{ view::binary::make( xid.data, xid.gtrid_length)};
            }

            type::branch branch( const xid_type& xid)
            {
               return type::branch{ view::binary::make( xid.data + xid.gtrid_length, xid.bqual_length)};
            }

            type::global global( const ID& id)
            {
               return global( id.xid);
            }

            type::branch branch( const ID& id)
            {
               return branch( id.xid);
            }
         } // range
      } // id

   } // common::transaction
} // casual
