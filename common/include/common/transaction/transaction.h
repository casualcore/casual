//!
//! transaction.h
//!
//! Created on: Oct 20, 2014
//!     Author: Lazan
//!

#ifndef CASUAL_COMMON_TRANSACTION_TRANSACTION_H_
#define CASUAL_COMMON_TRANSACTION_TRANSACTION_H_


#include "common/transaction/id.h"

#include "tx.h"

#include <ostream>
#include <vector>

namespace casual
{
   namespace common
   {
      namespace transaction
      {
         class Transaction
         {
         public:

            typedef TRANSACTION_STATE state_type;
            enum class State : state_type
            {
               active = TX_ACTIVE,
               rollback = TX_ROLLBACK_ONLY,
               timeout = TX_TIMEOUT_ROLLBACK_ONLY,
               suspended,
               inactive,
            };


            inline void state( State state)
            {
               m_previous = m_state;
               m_state = state;

            }

            inline State state() const
            {
               return m_state;
            }

            inline State previous() const
            {
               return m_previous;
            }

            explicit operator bool() const { return trid && m_state != State::suspended;}

            ID trid;

            //!
            //! associated rm:s to this transaction
            //!
            std::vector< int> associated;


            friend std::ostream& operator << ( std::ostream& out, const Transaction& rhs)
            {
               return out << "{trid: " << rhs.trid << ", state: " << rhs.m_state << ", previous: " << rhs.m_previous << "}";
            }

         private:
            State m_state = State::inactive;
            State m_previous = State::inactive;
         };
      }
   }

} // casual

#endif // TRANSACTION_H_
