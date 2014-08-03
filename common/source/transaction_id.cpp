//!
//! transaction_id.cpp
//!
//! Created on: Nov 1, 2013
//!     Author: Lazan
//!

#include "common/transaction_id.h"
#include "common/uuid.h"



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
            m_xid.formatID = Format::cNull;
            m_xid.gtrid_length = 0;
            m_xid.bqual_length = 0;
         }

         ID::ID( const XID& xid)
         {
            memcpy( &m_xid, &xid, sizeof( XID));
         }


         ID::ID( const Uuid& gtrid, const Uuid& bqual)
         {
            local::casualXid( m_xid, gtrid, bqual);
         }


         ID::ID( ID&& rhs) noexcept
         {
            m_xid = rhs.m_xid;
            rhs.m_xid.formatID = Format::cNull;

         }
         ID& ID::operator = ( ID&& rhs) noexcept
         {
            m_xid = rhs.m_xid;
            rhs.m_xid.formatID = Format::cNull;

            return *this;
         }

         ID::ID( const ID& rhs)
         {
            memcpy( &m_xid, &rhs.m_xid, sizeof( XID));
         }

         ID& ID::operator = ( const ID& rhs)
         {
            memcpy( &m_xid, &rhs.m_xid, sizeof( XID));
            return *this;
         }

         ID ID::create()
         {
            return ID( Uuid::make(), Uuid::make());
         }

         void ID::generate()
         {
            local::casualXid( m_xid, Uuid::make(), Uuid::make());
         }

         ID ID::branch() const
         {
            ID result( *this);

            if( result)
            {
               Uuid bqual = Uuid::make();

               auto size = sizeof( Uuid::uuid_type);

               memcpy( result.m_xid.data + result.m_xid.gtrid_length, bqual.get(), size);

            }

            return result;
         }



         bool ID::null() const
         {
            return m_xid.formatID == Format::cNull;
         }

         ID::operator bool() const
         {
            return ! null();
         }

         const XID& ID::xid() const
         {
            return m_xid;
         }


         XID& ID::xid()
         {
            return m_xid;
         }

         namespace local
         {
            namespace
            {
               namespace string
               {

                  char hex( char value)
                  {
                     if( value < 10)
                     {
                        return value + 48;
                     }
                     return value + 87;
                  }


                  void generic( const char* first, const char* last, std::string& result)
                  {
                     while( first != last)
                     {
                        result.push_back( hex( ( 0xf0 & *first) >> 4));
                        result.push_back( hex( 0x0f & *first));

                        ++first;
                     }

                  }

                  std::string generic( const char* first, const char* last)
                  {

                     std::string result;

                     auto size = last - first;
                     result.reserve( size + 4);

                     switch( size)
                     {
                        case 16:
                        {
                           generic( first, first + 4, result);
                           result.push_back( '-');
                           first += 4;
                        }
                        case 12:
                        {
                           generic( first, first + 2, result);
                           result.push_back( '-');
                           generic( first + 2, first + 4, result);
                           result.push_back( '-');
                           first += 4;
                        }
                        case 8:
                        {
                           generic( first, first + 2, result);
                           result.push_back( '-');
                           first += 2;
                        }
                        default:
                        {
                           generic( first, last, result);
                           break;
                        }
                     }
                     return result;
                  }

               } // string

            } // <unnamed>
         } // local

         std::string ID::stringGlobal() const
         {

            switch( m_xid.formatID)
            {
               /*
               case ID::cCasual:
               {
                  return local::string::fromUuid( m_xid.data, m_xid.data + m_xid.gtrid_length);
               }
               */
               default:
               {
                  return local::string::generic( m_xid.data, m_xid.data + m_xid.gtrid_length);
               }
               case ID::cNull:
               {
                  return {};
               }
            }

         }

         std::string ID::stringBranch() const
         {
            auto first = std::begin( m_xid.data) +  m_xid.gtrid_length;
            auto last = first +  m_xid.bqual_length;

            switch( m_xid.formatID)
            {
               /*
               case ID::cCasual:
               {
                  return local::string::fromUuid( first, last);
               }
               */
               default:
               {
                  return local::string::generic( first, last);
               }
               case ID::cNull:
               {
                  return {};
               }
            }
         }


         bool operator < ( const ID& lhs, const ID& rhs)
         {
            return std::lexicographical_compare(
               std::begin( lhs.m_xid.data), std::begin( lhs.m_xid.data) + lhs.m_xid.gtrid_length + lhs.m_xid.bqual_length,
               std::begin( rhs.m_xid.data), std::begin( rhs.m_xid.data) + rhs.m_xid.gtrid_length + rhs.m_xid.bqual_length);
         }

         bool operator == ( const ID& lhs, const ID& rhs)
         {
            return lhs.m_xid.gtrid_length == rhs.m_xid.gtrid_length && lhs.m_xid.bqual_length == rhs.m_xid.bqual_length &&
               std::equal(
                 std::begin( lhs.m_xid.data), std::begin( lhs.m_xid.data) + lhs.m_xid.gtrid_length + lhs.m_xid.bqual_length,
                 std::begin( rhs.m_xid.data));
         }


      } // transaction
   } // common
} // casual
