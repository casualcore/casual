//!
//! transaction_id.cpp
//!
//! Created on: Nov 1, 2013
//!     Author: Lazan
//!

#include "common/transaction/id.h"
#include "common/uuid.h"
#include "common/process.h"



#include <ios>
#include <sstream>
#include <iomanip>

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


         ID::ID()
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
            return ID( Uuid::make(), Uuid::make(), std::move( owner));
         }

         ID ID::create()
         {
            return ID( Uuid::make(), Uuid::make(), process::handle());
         }


         ID ID::branch() const
         {
            ID result( *this);

            if( result)
            {
               Uuid bqual = Uuid::make();

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
            return ! null();
         }



         xid_range_type ID::range() const
         {
            return { xid.data, xid.data + xid.gtrid_length + xid.bqual_length};
         }


         const process::Handle& ID::owner() const
         {
            return m_owner;
         }


         namespace local
         {
            namespace
            {
               namespace stream
               {

                  inline char hex( char value)
                  {
                     if( value < 10)
                     {
                        return value + 48;
                     }
                     return value + 87;
                  }


                  inline std::ostream& generic_hex( std::ostream& out, const char* first, const char* last)
                  {
                     while( first != last)
                     {
                        out << hex( ( 0xf0 & *first) >> 4);
                        out << hex( 0x0f & *first);

                        ++first;
                     }
                     return out;
                  }

                  inline std::ostream& generic( std::ostream& out, const char* first, const char* last)
                  {
                     auto size = last - first;

                     switch( size)
                     {
                        case 16:
                        {
                           generic_hex( out, first, first + 4) << '-';
                           first += 4;
                        }
                        case 12:
                        {
                           generic_hex( out, first, first + 2) << '-';
                           generic( out, first + 2, first + 4) << '-';
                           first += 4;
                        }
                        case 8:
                        {
                           generic_hex( out, first, first + 2) << '-';
                           first += 2;
                        }
                        default:
                        {
                           generic_hex( out, first, last);
                           break;
                        }
                     }
                     return out;
                  }
               } // stream
            } // <unnamed>
         } // local


         std::ostream& operator << ( std::ostream& out, const ID& id)
         {
            if( out && id)
            {
               local::stream::generic( out, id.xid.data, id.xid.data + id.xid.gtrid_length) << ':';
               local::stream::generic( out, id.xid.data + id.xid.gtrid_length, id.xid.data + id.xid.gtrid_length + id.xid.bqual_length)
                  << ':' << id.m_owner.pid << ':' << id.m_owner.queue;
            }
            return out;
         }


         bool operator < ( const ID& lhs, const ID& rhs)
         {
            return std::lexicographical_compare(
               std::begin( lhs.xid.data), std::begin( lhs.xid.data) + lhs.xid.gtrid_length + lhs.xid.bqual_length,
               std::begin( rhs.xid.data), std::begin( rhs.xid.data) + rhs.xid.gtrid_length + rhs.xid.bqual_length);
         }

         bool operator == ( const ID& lhs, const ID& rhs)
         {
            return lhs.xid.gtrid_length == rhs.xid.gtrid_length && lhs.xid.bqual_length == rhs.xid.bqual_length &&
               std::equal(
                 std::begin( lhs.xid.data), std::begin( lhs.xid.data) + lhs.xid.gtrid_length + lhs.xid.bqual_length,
                 std::begin( rhs.xid.data));
         }


         xid_range_type global( const ID& id)
         {
            return { id.xid.data, id.xid.data + id.xid.gtrid_length};
         }

         xid_range_type branch( const ID& id)
         {
            return { id.xid.data + id.xid.gtrid_length,
                  id.xid.data + id.xid.gtrid_length + id.xid.bqual_length};
         }


      } // transaction
   } // common
} // casual
