//!
//! transaction_id.h
//!
//! Created on: Nov 1, 2013
//!     Author: Lazan
//!

#ifndef TRANSACTION_ID_H_
#define TRANSACTION_ID_H_

#include <tx.h>


#include "common/uuid.h"

#include "common/algorithm.h"


#include <string>
#include <ostream>



namespace casual
{
   namespace common
   {
      namespace transaction
      {
         class ID
         {
         public:

            enum Format
            {
               cNull = -1,
               cCasual = 42
            };

            //!
            //! Initialize with null-xid
            //!
            ID();

            ID( const XID& xid);


            //!
            //! Initialize with uuid, gtrid and bqual.
            //! Sets the format id to "casual"
            //!
            //! @note not likely to be used other than unittesting
            //!
            ID( const Uuid& gtrid, const Uuid& bqual);


            ID( ID&&) noexcept;
            ID& operator = ( ID&&) noexcept;


            ID( const ID&);
            ID& operator = ( const ID&);

            //!
            //! Creates a new unique transaction id, global and branch
            //!
            static ID create();

            //!
            //! Generate a new XID
            //!
            void generate();

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
            //! @return xa XID
            //!
            //! @{
            const XID& xid() const;
            XID& xid();
            //! @}



            std::string stringGlobal() const;

            std::string stringBranch() const;


            friend std::ostream& operator << ( std::ostream& out, const ID& id)
            {
               if( out && id)
               {
                  out << id.stringGlobal() << ":" << id.stringBranch();
               }
               return out;
            }

            friend bool operator < ( const ID& lhs, const ID& rhs);

            friend bool operator == ( const ID& lhs, const ID& rhs);

            friend bool operator != ( const ID& lhs, const ID& rhs)
            {
               return ! ( lhs == rhs);
            }



         private:

            XID m_xid;
         };

         namespace internal
         {
            inline const XID& xid( const XID& value) { return value;}

            inline const XID& xid( const ID& value) { return value.xid();}
         } // internal


         //!
         //! @return a (binary) range that represent the global part of the xid
         //!
         template< typename T>
         inline auto global( T&& id) -> decltype( range::make( internal::xid( id).data))
         {
            auto& xid = internal::xid( id);
            using range_type = decltype( range::make( xid.data));

            return range_type( xid.data, xid.data + xid.gtrid_length);
         }

         //!
         //! @return a (binary) range that represent the branch part of the xid
         //!
         template< typename T>
         inline auto branch( T&& id) -> decltype( range::make( internal::xid( id).data))
         {
            auto& xid = internal::xid( id);
            using range_type = decltype( range::make( xid.data));

            auto branchBegin = xid.data + xid.gtrid_length;

            return range_type( branchBegin, branchBegin + xid.bqual_length);
         }



         //!
         //! Overload for transaction::Id
         //!
         //! @{
         template< typename M>
         void casual_marshal_value( ID& value, M& marshler)
         {
            marshler << value.xid().formatID;

            if( value)
            {
               marshler << value.xid().gtrid_length;
               marshler << value.xid().bqual_length;

               marshler.append(
                  std::begin( value.xid().data),
                  std::begin( value.xid().data) + value.xid().gtrid_length + value.xid().bqual_length);
            }
         }

         template< typename M>
         void casual_unmarshal_value( ID& value, M& unmarshler)
         {
            unmarshler >> value.xid().formatID;

            if( value)
            {
               unmarshler >> value.xid().gtrid_length;
               unmarshler >> value.xid().bqual_length;

               unmarshler.consume(
                  std::begin( value.xid().data),
                  value.xid().gtrid_length + value.xid().bqual_length);
            }

         }
         //! @}

      } // transaction
   } // common
} // casual




#endif // TRANSACTION_ID_H_
