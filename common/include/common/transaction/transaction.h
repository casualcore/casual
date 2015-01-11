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
               //suspended = - 1,
               active = TX_ACTIVE,
               timeout = TX_TIMEOUT_ROLLBACK_ONLY,
               rollback = TX_ROLLBACK_ONLY,
            };


            explicit operator bool() const { return trid && ! suspended;}

            ID trid;

            //!
            //! associated rm:s to this transaction
            //!
            std::vector< platform::resource::id_type> resources;

            //!
            //! associated descriptors to this transaction
            //!
            std::vector< platform::descriptor_type> descriptors;

            State state = State::active;
            bool suspended = true;


            void discard( platform::descriptor_type descriptor)
            {
               descriptors.erase(
                     std::remove( std::begin( descriptors), std::end( descriptors), descriptor),
                     std::end( descriptors)
               );
            }


            friend bool operator == ( const Transaction& lhs, const ID& rhs) { return lhs.trid == rhs;}

            friend std::ostream& operator << ( std::ostream& out, const Transaction& rhs)
            {
               return out << "{trid: " << rhs.trid << ", state: " << rhs.state <<
                     ", suspended: " << std::boolalpha << rhs.suspended <<
                     ", resources: " << range::make( rhs.resources) <<
                     ", descriptors: " << range::make( rhs.descriptors) << "}";
            }
         };
      }
   }

} // casual

#endif // TRANSACTION_H_
