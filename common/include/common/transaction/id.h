//!
//! transaction_id.h
//!
//! Created on: Nov 1, 2013
//!     Author: Lazan
//!

#ifndef CASUAL_COMMON_TRANSACTION_ID_H_
#define CASUAL_COMMON_TRANSACTION_ID_H_

#include <tx.h>


#include "common/uuid.h"

#include "common/algorithm.h"
#include "common/process.h"


#include <string>
#include <ostream>


//!
//! Global scope compare operations for XID
//!
//! @{
bool operator == ( const XID& lhs, const XID& rhs);
bool operator < ( const XID& lhs, const XID& rhs);
bool operator != ( const XID& lhs, const XID& rhs);
//! @}

//!
//! Global stream opaerator for XID
//!
std::ostream& operator << ( std::ostream& out, const XID& xid);

namespace casual
{
   namespace common
   {
      namespace transaction
      {
         using xid_range_type = decltype( range::make( std::declval< const XID&>().data));


         class ID
         {
         public:

            enum Format
            {
               cNull = -1,
               cCasual = 2
            };

            //!
            //! Creates a new unique transaction id, global and branch
            //!
            static ID create();
            static ID create( process::Handle owner);


            //!
            //! Initialize with null-xid
            //! @{
            ID();
            ID( process::Handle owner);
            //! @}

            explicit ID( const XID& xid);


            //!
            //! Initialize with uuid, gtrid and bqual.
            //! Sets the format id to "casual"
            //!
            //! @note not likely to be used other than unittesting
            //!
            ID( const Uuid& gtrid, const Uuid& bqual, process::Handle owner);


            ID( ID&&) noexcept;
            ID& operator = ( ID&&) noexcept;


            ID( const ID&);
            ID& operator = ( const ID&);



            //!
            //! Creates a new Id with same global transaction id but a new branch id.
            //!
            ID branch() const;


            //!
            //! @return true if XID is null
            //!
            bool null() const;

            //!
            //! @return true if XID is not null
            //!
            explicit operator bool() const;



            //!
            //! Return the raw data of the xid in a range
            //!
            xid_range_type range() const;


            //!
            //! @return owner/creator of the transaction
            //!
            const process::Handle& owner() const;





            friend bool operator < ( const ID& lhs, const ID& rhs);
            friend bool operator == ( const ID& lhs, const ID& rhs);
            friend bool operator == ( const ID& lhs, const XID& rhs);
            inline friend bool operator != ( const ID& lhs, const ID& rhs)
            {
               return ! ( lhs == rhs);
            }

            template< typename M>
            friend void casual_marshal_value( const ID& value, M& marshler);

            template< typename M>
            friend void casual_unmarshal_value( ID& value, M& unmarshler);


            //!
            //! The XA-XID object.
            //!
            //! We need to have access to the xid to communicate via xa and such,
            //! no reason to keep it private and have getters..
            //!
            XID xid;


            friend std::ostream& operator << ( std::ostream& out, const ID& id);

         private:

            process::Handle m_owner;

         };


         //!
         //! @return a (binary) range that represent the global part of the xid
         //!
         //! @{
         xid_range_type global( const ID& id);
         xid_range_type global( const XID& id);
         //! @}

         //!
         //! @return a (binary) range that represent the branch part of the xid
         //!
         //! @{
         xid_range_type branch( const ID& id);
         xid_range_type branch( const XID& id);
         //! @}

         //!
         //! @return true if trid is null, false otherwise
         //!
         //! @{
         bool null( const ID& id);
         bool null( const XID& id);
         //! @}


         //!
         //! Overload for transaction::Id
         //!
         //! @{
         template< typename M>
         void casual_marshal_value( const ID& value, M& marshler)
         {
            marshler << value.xid.formatID;

            if( value)
            {
               marshler << value.m_owner;

               marshler << value.xid.gtrid_length;
               marshler << value.xid.bqual_length;

               marshler.append( value.range());
            }
         }

         template< typename M>
         void casual_unmarshal_value( ID& value, M& unmarshler)
         {
            unmarshler >> value.xid.formatID;

            if( value)
            {
               unmarshler >> value.m_owner;

               unmarshler >> value.xid.gtrid_length;
               unmarshler >> value.xid.bqual_length;

               unmarshler.consume(
                  std::begin( value.xid.data),
                  value.xid.gtrid_length + value.xid.bqual_length);
            }

         }
         //! @}

      } // transaction
   } // common
} // casual




#endif // TRANSACTION_ID_H_
