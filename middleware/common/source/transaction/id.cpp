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
   if( ! casual::common::transaction::id::null( xid))
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
                  xid.gtrid_length = memory::size( bqual.get());
                  xid.bqual_length = xid.gtrid_length;

                  algorithm::copy( gtrid.get(), xid.data);
                  algorithm::copy( bqual.get(), xid.data + xid.gtrid_length);

                  xid.formatID = ID::Format::casual;
               }

            } // <unnamed>
         } // local

         ID::ID() noexcept
         {
            xid.formatID = Format::null;
         }



         ID::ID( const process::Handle& owner) : m_owner( std::move( owner))
         {
            xid.formatID = Format::null;
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
            rhs.xid.formatID = Format::null;

         }
         ID& ID::operator = ( ID&& rhs) noexcept
         {
            xid = rhs.xid;
            m_owner = std::move( rhs.m_owner);
            rhs.xid.formatID = Format::null;

            return *this;
         }

         bool ID::null() const
         {
            return xid.formatID == Format::null;
         }

         ID::operator bool() const
         {
            return ! id::null( xid);
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
               return null( id.xid);
            }

            bool null( const xid_type& id)
            {
               return id.formatID == ID::Format::null;
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
               range_type global( const xid_type& xid)
               {
                  return range_type{ xid.data, xid.data + xid.gtrid_length};
               }

               range_type branch( const xid_type& xid)
               {
                  return range_type{ xid.data + xid.gtrid_length,
                        xid.data + xid.gtrid_length + xid.bqual_length};
               }

               range_type data( const xid_type& xid)
               {
                  return range_type{ xid.data, xid.data + xid.gtrid_length + xid.bqual_length};
               }

               range_type data( const ID& id)
               {
                  return data( id.xid);
               }

               range_type global( const ID& id)
               {
                  return global( id.xid);
               }

               range_type branch( const ID& id)
               {
                  return branch( id.xid);
               }
            } // range
         } // id
      } // transaction
   } // common
} // casual
