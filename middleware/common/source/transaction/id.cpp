//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/transaction/id.h"
#include "common/uuid.h"
#include "common/process.h"
#include "common/transcode.h"


#include <ios>
#include <sstream>
#include <iomanip>
#include <iostream>


std::ostream& operator << ( std::ostream& out, const XID& xid)
{
   if( xid.formatID != casual::common::transaction::ID::Format::null)
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

      casual::common::transcode::hex::encode( out, casual::common::binary::span::make( xid.data, xid.gtrid_length)); 
      out  << ':';
      casual::common::transcode::hex::encode( out, casual::common::binary::span::make( xid.data + xid.gtrid_length, xid.bqual_length));
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

               algorithm::copy( gtrid, binary::span::make( xid.data, std::ssize( gtrid)));
               algorithm::copy( bqual, binary::span::make( xid.data + xid.gtrid_length, std::ssize( bqual)));

               xid.formatID = ID::Format::casual;
            }

            platform::binary::type create_data( auto&& gtrid, auto&& bqual)
            {
               platform::binary::type data;
               data.resize( std::size( gtrid) + std::size( bqual));

               algorithm::copy( gtrid, std::begin( data));
               algorithm::copy( bqual, std::begin( data) + std::size( gtrid));

               return data;
            }

            platform::binary::type xid_to_data( const XID& xid)
            {
               if( xid.formatID == ID::Format::null)
                  return {};

               auto span = binary::span::make( xid.data, xid.data + xid.gtrid_length + xid.bqual_length);

               return platform::binary::type{ std::begin( span), std::end( span)};
            }

         } // <unnamed>
      } // local

      ID::ID( strong::process::id owner) : m_owner( std::move( owner))
      {}

      ID::ID( const ::XID& xid)
         :  m_format_id( xid.formatID),
            m_data{ local::xid_to_data( xid)},
            m_bqual_pivot( static_cast< short>( xid.gtrid_length))
      {
      }

      ID::ID( global::id::range gtrid)
         : m_format_id( Format::casual),
            m_data( local::create_data( gtrid, uuid::make().range())),
            m_bqual_pivot( static_cast< short>( gtrid.size()))
      {
      }

      ID::ID( Uuid gtrid, Uuid bqual, strong::process::id owner) 
         : m_format_id( Format::casual),
            m_data( local::create_data( gtrid.range(), bqual.range())),
            m_bqual_pivot( static_cast< short>( gtrid.range().size())),
            m_owner( std::move( owner))
      {
      }

      ID::ID( global::id::range gtrid, id::range::type::branch bqual, long format_id)
         : m_format_id( format_id),
            m_data( local::create_data( gtrid, bqual)),
            m_bqual_pivot( static_cast< short>( gtrid.size()))
      {

      }

      ID::ID( ID&& rhs) noexcept = default;
      ID& ID::operator = ( ID&& rhs) noexcept = default;

      bool ID::null() const
      {
         return m_format_id == Format::null;
      }

      ID::operator bool() const
      {
         return ! null(); 
      }


      strong::process::id ID::owner() const
      {
         return m_owner;
      }

      void ID::owner( strong::process::id handle)
      {
         m_owner = handle;
      }

      XID ID::to_xid() const
      {
         XID xid{};
         xid.formatID = m_format_id;
         xid.gtrid_length = global().size();
         xid.bqual_length = branch().size();

         auto string_like = binary::span::to_string_like( data());

         std::copy( std::begin( string_like), std::end( string_like), std::begin( xid.data));
         return xid;
      }

      bool operator < ( const ID& lhs, const ID& rhs)
      {
         if( lhs.m_format_id != rhs.m_format_id) 
            return lhs.m_format_id < rhs.m_format_id;
         if( lhs.m_format_id == ID::Format::null) 
            return false;

         return std::ranges::lexicographical_compare( lhs.m_data, rhs.m_data);
      }

      bool operator == ( const ID& lhs, const ID& rhs)
      {
         if( lhs.m_format_id != rhs.m_format_id) 
            return false;
         if( lhs.m_format_id == ID::Format::null) 
            return true;

         return std::ranges::equal( lhs.m_data, rhs.m_data);
      }

      bool operator == ( const ID& lhs, const ::XID& rhs)
      {
         if( lhs.m_format_id != rhs.formatID) 
            return false;
         if( lhs.m_format_id == ID::Format::null)
            return true;

         return algorithm::equal( lhs.data(), binary::span::make( rhs.data, rhs.data + rhs.gtrid_length + rhs.bqual_length));
      }


      bool operator == ( const ID& lhs, global::id::range rhs)
      {
         return std::ranges::equal( lhs.global(), rhs);
      }

      std::ostream& operator << ( std::ostream& out, const ID& id)
      {
         transcode::hex::encode( out, id.global());
         out  << ':';
         transcode::hex::encode( out, id.branch());
         
         return out << ':' << id.m_format_id
            << ':' << id.m_owner;  
      }

      namespace id
      {
         ID branch( const ID& id)
         {
            return ID{ id.global(), uuid::make().range(), id.format()};
         }

         ID create( strong::process::id owner)
         {
            return ID{ uuid::make(), uuid::make(), owner};
         }

         ID create()
         {
            return create( process::id());
         }

      } // id
         

   } // common::transaction
} // casual
