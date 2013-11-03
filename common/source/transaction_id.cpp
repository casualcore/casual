//!
//! transaction_id.cpp
//!
//! Created on: Nov 1, 2013
//!     Author: Lazan
//!

#include "common/transaction_id.h"
#include "common/uuid.h"

namespace casual
{
   namespace common
   {
      namespace transaction
      {

         ID::ID()
         {
            m_xid.formatID = Format::cNull;
         }



         ID::ID( const ID&) = default;

         ID::ID( ID&&) = default;
         ID& ID::operator = ( ID&&) = default;

         ID& ID::operator = ( const ID&) = default;

         ID ID::create()
         {
            ID result;

            result.generate();

            return result;
         }

         void ID::generate()
         {
            m_xid.formatID = Format::cCasual;

            auto gtrid = Uuid::make();
            auto bqual = Uuid::make();

            std::copy( std::begin( gtrid.get()), std::end( gtrid.get()), std::begin( m_xid.data));
            m_xid.gtrid_length = std::distance(std::begin( gtrid.get()), std::end( gtrid.get()));

            std::copy( std::begin( bqual.get()), std::end( bqual.get()), std::begin( m_xid.data) + m_xid.gtrid_length);
            m_xid.bqual_length = m_xid.gtrid_length;

         }

         ID ID::branch() const
         {
            ID result( *this);

            if( result)
            {
               // TODO: create branch
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
                  template< typename Iter>
                  std::string generic( Iter first, std::size_t size)
                  {
                     return {};
                  }


                  template< typename Iter>
                  std::string fromUuid( Iter first, std::size_t size)
                  {
                     if( size == sizeof( Uuid::uuid_type))
                     {
                        return Uuid::toString( reinterpret_cast< const Uuid::uuid_type&>( first));
                     }
                     else
                     {
                        return string::generic( first, size);
                     }
                  }
               } // string

            } // <unnamed>
         } // local

         std::string ID::stringGlobal() const
         {

            switch( m_xid.formatID)
            {
               case ID::cCasual:
               {
                  return local::string::fromUuid( std::begin( m_xid.data), m_xid.gtrid_length);
               }
               default:
               {
                  return local::string::generic( std::begin( m_xid.data), m_xid.gtrid_length);
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

            switch( m_xid.formatID)
            {
               case ID::cCasual:
               {
                  return local::string::fromUuid( first, m_xid.bqual_length);
               }
               default:
               {
                  return local::string::generic( first, m_xid.bqual_length);
               }
               case ID::cNull:
               {
                  return {};
               }
            }
         }


         bool ID::operator < (const ID& rhs) const
         {
            return std::lexicographical_compare(
               std::begin( m_xid.data), std::begin( m_xid.data) + m_xid.gtrid_length + m_xid.bqual_length,
               std::begin( rhs.m_xid.data), std::begin( rhs.m_xid.data) + rhs.m_xid.gtrid_length + rhs.m_xid.bqual_length);
         }

         bool ID::operator == (const ID& rhs) const
         {
            return m_xid.gtrid_length + m_xid.bqual_length == rhs.m_xid.gtrid_length + rhs.m_xid.bqual_length &&
               std::equal(
                 std::begin( m_xid.data), std::begin( m_xid.data) + m_xid.gtrid_length + m_xid.bqual_length,
                 std::begin( rhs.m_xid.data));
         }

         namespace string
         {
            std::string global( const ID& id)
            {
               std::string result;


               return result;
            }

            std::string branch( const ID& id)
            {
               // TODO:
               return {};
            }

         } // string

      } // transaction
   } // common
} // casual
