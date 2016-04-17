//!
//! transaction.h
//!
//! Created on: Oct 20, 2014
//!     Author: Lazan
//!

#ifndef CASUAL_COMMON_TRANSACTION_TRANSACTION_H_
#define CASUAL_COMMON_TRANSACTION_TRANSACTION_H_


#include "common/transaction/id.h"
#include "common/timeout.h"
#include "common/platform.h"


#include "tx.h"

#include <iosfwd>
#include <vector>
#include <chrono>

namespace casual
{
   namespace common
   {
      namespace transaction
      {
         class Transaction
         {
         public:

            enum class State : TRANSACTION_STATE
            {
               //suspended = - 1,
               active = TX_ACTIVE,
               timeout = TX_TIMEOUT_ROLLBACK_ONLY,
               rollback = TX_ROLLBACK_ONLY,
            };


            Transaction();
            Transaction( ID trid);


            ID trid;

            Timeout timout;

            //!
            //! associated rm:s to this transaction
            //!
            std::vector< platform::resource::id::type> resources;




            State state = State::active;


            explicit operator bool() const;


            //!
            //! associate a descriptor to this transaction
            //!
            void associate( platform::descriptor_type descriptor);


            //!
            //! discards a descriptor from this transaction
            //!
            void discard( platform::descriptor_type descriptor);


            //!
            //! @return true if this transaction has any pending replies
            //! associated
            //!
            bool associated() const;

            //!
            //! @return true if this transaction has @p descriptor associated
            //!
            bool associated( platform::descriptor_type descriptor) const;


            //!
            //! associated descriptors to this transaction
            //!
            const std::vector< platform::descriptor_type>& descriptors() const;

            //!
            //! @return true if the transaction never had any associated descriptors
            //!
            bool local() const;


            friend bool operator == ( const Transaction& lhs, const ID& rhs);
            friend bool operator == ( const Transaction& lhs, const XID& rhs);
            friend std::ostream& operator << ( std::ostream& out, const Transaction& rhs);

         private:


            std::vector< platform::descriptor_type> m_descriptors;

            bool m_local = true;


         };

      } // transaction
   } // common
} // casual

#endif // TRANSACTION_H_
