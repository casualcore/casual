//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/transaction/global.h"

#include "common/uuid.h"

#include "common/view/binary.h"
#include "common/algorithm.h"
#include "common/process.h"

#include "common/serialize/value/customize.h"

#include <tx.h>

#include <string>
#include <ostream>


//! Global scope compare operations for XID
//! @{
bool operator == ( const XID& lhs, const XID& rhs);
bool operator < ( const XID& lhs, const XID& rhs);
inline bool operator != ( const XID& lhs, const XID& rhs) { return ! ( lhs == rhs);}
//! @}

//! Global stream operator for XID
std::ostream& operator << ( std::ostream& out, const XID& xid);

namespace casual
{
   namespace common
   {
      namespace transaction
      {
         using xid_type = XID;


         namespace xid
         {
            XID null() noexcept;
            bool null( const XID& id) noexcept;
         } // xid

         class ID
         {
         public:

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
            explicit ID( const process::Handle& owner);
            //! @}

            explicit ID( const xid_type& xid);

            explicit ID( const global::ID& gtrid);

            //! Initialize with uuid, gtrid and bqual.
            //! Sets the format id to "casual"
            //!
            //! @note not likely to be used other than unittesting
            ID( Uuid gtrid, Uuid bqual, const process::Handle& owner);

            ID( ID&&) noexcept;
            ID& operator = ( ID&&) noexcept;

            ID( const ID&) noexcept = default;
            ID& operator = ( const ID&) noexcept = default;


            //! @return true if XID is null
            bool null() const;

            //! @return true if XID is not null
            explicit operator bool() const;


            //! @return owner/creator of the transaction
            const process::Handle& owner() const;
            void owner( const process::Handle& handle);

            friend bool operator < ( const ID& lhs, const ID& rhs);
            friend bool operator == ( const ID& lhs, const ID& rhs);
            friend bool operator == ( const ID& lhs, const xid_type& rhs);
            inline friend bool operator != ( const ID& lhs, const ID& rhs)
            {
               return ! ( lhs == rhs);
            }

            friend bool operator == ( const ID& lhs, const global::ID& rhs);
            friend bool operator == ( const global::ID& lhs, const ID& rhs) { return rhs == lhs;}

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE_NAME( m_owner, "owner");
               CASUAL_SERIALIZE( xid);
            )


            //! The XA-XID object.
            //!
            //! We need to have access to the xid to communicate via xa and such,
            //! no reason to keep it private and have getters..
            xid_type xid = xid::null();

            friend std::ostream& operator << ( std::ostream& out, const ID& id);

         private:
            process::Handle m_owner;
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
            ID create( const process::Handle& owner);

            //! @return true if trid is null, false otherwise
            //! @{
            bool null( const ID& id);
            //! @}

            //! Creates a new Id with same global transaction id but a new branch id.
            //! if the transaction is _null_ then a _null_ xid is returned 
            ID branch( const ID& id);
            
            namespace range
            {
               namespace type
               {
                  using global = transaction::global::id::range;

                  struct branch_tag{};
                  using branch = strong::Span< const char, branch_tag>;
               } // type

               namespace detail
               {
                  template< typename X>
                  auto data( X&& xid) 
                  {
                     return view::binary::make( xid.data, xid.data + xid.gtrid_length + xid.bqual_length);
                  }
               } // detail

               //! @return a (binary) range that represent the data part of the xid, global + branch
               //! @{
               inline auto data( const xid_type& xid) { return detail::data( xid);}
               inline auto data( const ID& id) { return data( id.xid);}
               inline auto data( xid_type& xid) { return detail::data( xid);}
               inline auto data( ID& id) { return data( id.xid);}
               //! @}

               //! @return a (binary) range that represent the global part of the xid
               //! @{
               type::global global( const ID& id);
               type::global global( const xid_type& id);
               //! @}

               //! @return a (binary) range that represent the branch part of the xid
               //! @{
               type::branch branch( const ID& id);
               type::branch branch( const xid_type& id);
               //! @}

            } // range
         } // id
         
      } // transaction

      namespace serialize::customize::composite
      {
         //! specialization for XID
         template< typename A>
         struct Value< XID, A>
         {
            template< typename V>  
            static void serialize( A& archive, V&& xid)
            {
               CASUAL_SERIALIZE_NAME( xid.formatID, "formatID");

               if( ! transaction::xid::null( xid))
               {
                  CASUAL_SERIALIZE_NAME( xid.gtrid_length, "gtrid_length");
                  CASUAL_SERIALIZE_NAME( xid.bqual_length, "bqual_length");
                  CASUAL_SERIALIZE_NAME( transaction::id::range::data( xid), "data");
               }
            }
         };

      } // serialize::customize::composite

   } // common
} // casual





