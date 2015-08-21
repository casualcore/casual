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
            std::vector< platform::resource::id_type> resources;

            //!
            //! associated descriptors to this transaction
            //!
            std::vector< platform::descriptor_type> descriptors;


            State state = State::active;


            explicit operator bool() const;

            //!
            //! discards a descriptor from this transaction
            //!
            void discard( platform::descriptor_type descriptor);


            friend bool operator == ( const Transaction& lhs, const ID& rhs);
            friend bool operator == ( const Transaction& lhs, const XID& rhs);
            friend std::ostream& operator << ( std::ostream& out, const Transaction& rhs);
         };

      } // transaction
   } // common
} // casual

#endif // TRANSACTION_H_
