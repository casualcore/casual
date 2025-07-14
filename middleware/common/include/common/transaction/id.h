//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/transaction/global.h"

#include "common/uuid.h"

#include "common/binary/span.h"
#include "common/algorithm.h"
#include "common/process.h"


#include <tx.h>

#include <string>
#include <ostream>


//! Global stream operator for XID
std::ostream& operator << ( std::ostream& out, const XID& xid);

namespace casual
{
   namespace common::transaction
   {

      namespace xid
      {
         bool null( const XID& id) noexcept;
      } // xid

      namespace id
      {
         namespace range::type
         {
            using global = transaction::global::id::range;

            struct branch_tag{};
            using branch = strong::Span< const std::byte, branch_tag>;
         } // range::type

      } // id

      struct ID
      {
         struct Format
         {
            enum 
            {
               null = -1,
               casual = 42,
               branch = 43,
            };
         };

         //! Initialize with null-xid
         //! @{
         ID() noexcept = default;
         explicit ID( strong::process::id owner);
         //! @}

         explicit ID( const ::XID& xid);

         //! creates a new trid from gtrid and a generated bqual.
         explicit ID( global::id::range gtrid);

         explicit ID( global::id::range gtrid, id::range::type::branch bqual, long format_id = Format::casual);

         //! Initialize with uuid, gtrid and bqual.
         //! Sets the format id to "casual"
         //!
         //! @note not likely to be used other than unittesting
         ID( Uuid gtrid, Uuid bqual, strong::process::id owner);

         ID( ID&&) noexcept;
         ID& operator = ( ID&&) noexcept;

         ID( const ID&) noexcept = default;
         ID& operator = ( const ID&) noexcept = default;

         long format() const noexcept { return m_format_id;}

         //! @return true if XID is null
         bool null() const;

         //! @return true if XID is not null
         explicit operator bool() const;


         //! @return owner/creator of the transaction
         strong::process::id owner() const;
         void owner( strong::process::id handle);

         friend bool operator < ( const ID& lhs, const ID& rhs);
         friend bool operator == ( const ID& lhs, const ID& rhs);
         friend bool operator == ( const ID& lhs, const ::XID& rhs);

         friend bool operator == ( const ID& lhs, global::id::range rhs);
         inline friend bool operator == ( const ID& lhs, const global::ID& rhs) { return lhs == rhs.range();}


         inline auto data() const { return binary::span::fixed::make( m_data.data(), m_data.size());}
         inline auto data() { return binary::span::fixed::make( m_data.data(), m_data.size());}
         inline auto global() const { return id::range::type::global( m_data.data(), m_bqual_pivot);}
         inline auto branch() const { return id::range::type::branch{ m_data.data() + m_bqual_pivot, m_data.data() + m_data.size()};}

         //! @return a XID object based on this ID
         XID to_xid() const;
         

         friend std::ostream& operator << ( std::ostream& out, const ID& id);

         CASUAL_CONST_CORRECT_SERIALIZE(
            
            CASUAL_SERIALIZE_NAME( m_format_id, "formatID");

            if( ! null())
            {
               long gtrid_length = global().size();
               long bqual_length = branch().size();

               CASUAL_SERIALIZE( gtrid_length);
               CASUAL_SERIALIZE( bqual_length);

               // maybe resize.
               resize( gtrid_length, bqual_length);

               CASUAL_SERIALIZE_NAME( data(), "data");

               if constexpr( ! serialize::archive::is::network::normalizing< std::decay_t< decltype( archive)>>)
               {
                  // we only serialize the owner if it't not over 'network'
                  CASUAL_SERIALIZE_NAME( m_owner, "owner");
               }
            }
         )

      private:

         inline void resize( platform::size::type gtrid_length, platform::size::type bqual_length) 
         {
               m_data.resize( gtrid_length + bqual_length);
               m_bqual_pivot = static_cast< short>( gtrid_length);
         }
         inline void resize( platform::size::type gtrid_length, platform::size::type bqual_length) const { /* no op*/}

         
         long m_format_id = Format::null;
         platform::binary::type m_data;

         // pivot between gtrid and bqual in m_data
         short m_bqual_pivot = {}; 
         //! owner/creator of the transaction
         strong::process::id m_owner;
      };

      namespace id
      {
         namespace max::size
         {
            inline constexpr auto global = MAXGTRIDSIZE;
            inline constexpr auto branch = MAXBQUALSIZE;
            
         } // max::size

         //! Creates a new unique transaction id, global and branch
         ID create();
         ID create( strong::process::id owner);

         //! Creates a new Id with same global transaction id but a new branch id.
         //! if the transaction is _null_ then a _null_ xid is returned 
         ID branch( const ID& id);

      } // id
      
   } // common::transaction
} // casual


namespace std 
{
   template<>
   struct hash< casual::common::transaction::ID>
   {
      std::size_t operator()( const casual::common::transaction::ID& value) const noexcept
      {
         auto range = value.data();
         return std::hash< std::string_view>{}( std::string_view( reinterpret_cast< const char*>( range.data()), range.size()));
      }
   };
}



