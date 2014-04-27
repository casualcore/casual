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


            bool operator < ( const ID& rhs) const;

            bool operator == ( const ID& rhs) const;

            bool operator != ( const ID& rhs) const
            {
               return ! ( *this == rhs);
            }


            std::string stringGlobal() const;

            std::string stringBranch() const;


         private:

            XID m_xid;
         };

         inline std::ostream& operator << ( std::ostream& out, const ID& value)
         {
            if( out.good())
            {
               out << value.stringGlobal() << "|" << value.stringBranch();
            }
            return out;
         }



         //!
         //! Overload for transaction::Id
         //!
         //! @{
         template< typename M>
         void casual_marshal_value( casual::common::transaction::ID& value, M& marshler)
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
         void casual_unmarshal_value( casual::common::transaction::ID& value, M& unmarshler)
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
