//!
//! transaction_id.h
//!
//! Created on: Nov 1, 2013
//!     Author: Lazan
//!

#ifndef TRANSACTION_ID_H_
#define TRANSACTION_ID_H_

#include <tx.h>

#include <string>



namespace casual
{
   namespace common
   {
      namespace transaction
      {
         class ID
         {
         public:

            //!
            //! Initialize with null-xid
            //!
            ID();

            ID( ID&&);
            ID& operator = ( ID&&);

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

            ID( const ID&);

            enum Format
            {
               cNull = -1,
               cCasual = 42
            };

            XID m_xid;
         };

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
