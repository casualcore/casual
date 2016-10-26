//!
//! casual
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
            //! associate a pending message reply
            //!
            void associate( const Uuid& correlation);


            //!
            //! discards a pending reply from this transaction
            //!
            void replied( const Uuid& correlation);


            //!
            //! @return true if this transaction has any pending replies
            //! associated
            //!
            bool pending() const;

            //!
            //! @return true if this transaction has @p correlation associated
            //!
            bool associated( const Uuid& correlation) const;


            //!
            //! associated descriptors to this transaction
            //!
            const std::vector< Uuid>& correlations() const;

            //!
            //! @return true if the transaction is only local
            //!
            bool local() const;


            //!
            //! Associate this transaction with 'external' resources. That is,
            //! make it "not local" so it will trigger a commit request to the TM
            //!
            void external();


            friend bool operator == ( const Transaction& lhs, const ID& rhs);
            friend bool operator == ( const Transaction& lhs, const XID& rhs);
            friend std::ostream& operator << ( std::ostream& out, const Transaction& rhs);

         private:

            std::vector< Uuid> m_pending;
            bool m_local = true;

         };

      } // transaction
   } // common
} // casual

#endif // TRANSACTION_H_
