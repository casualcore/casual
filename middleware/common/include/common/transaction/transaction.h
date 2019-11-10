//! Copyright (c) 2015, The casual project
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT


#pragma once



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
            friend std::ostream& operator << ( std::ostream& out, State value);


            Transaction();
            explicit Transaction( ID trid);

            ID trid;
            Timeout timout;
            State state = State::active;

            //! @return true if `trid` is _active_ 
            explicit operator bool() const;

            //! associated rm:s to this transaction
            inline const std::vector< strong::resource::id>& involved() const { return m_involved;}
            
            //! associate resource
            void involve( strong::resource::id id);

            template< typename R>
            auto involve( R&& range) -> std::enable_if_t< traits::is::iterable< R>::value>
            {
               for( auto id : range) involve( id);
            }  

            //! dynamic associated rm:s to this transaction, a subset to `involved`
            inline const std::vector< strong::resource::id>& dynamic() const { return m_dynamic;}
            
            //! associate id to dynamic registration
            //! @attention will not associate with involved
            //! @return true if id is not registred before
            bool associate_dynamic( strong::resource::id id);
            bool disassociate_dynamic( strong::resource::id id);

            //! associate a pending message reply
            void associate( const Uuid& correlation);

            //! discards a pending reply from this transaction
            void replied( const Uuid& correlation);

            //! @return true if this transaction has any pending replies
            //! associated
            bool pending() const;

            //! @return true if this transaction has @p correlation associated
            bool associated( const Uuid& correlation) const;

            //! associated descriptors to this transaction
            const std::vector< Uuid>& correlations() const;

            //! @return true if the transaction is only local, current process 
            //!  is the owner, no pending calls, and no external involvement
            bool local() const;


            //! Associate this transaction with 'external' resources. That is,
            //! make it "not local" so it will trigger a commit request to the TM
            void external();

            friend bool operator == ( const Transaction& lhs, const ID& rhs);
            friend bool operator == ( const Transaction& lhs, const XID& rhs);
            friend std::ostream& operator << ( std::ostream& out, const Transaction& rhs);

         private:
            std::vector< strong::resource::id> m_involved;
            std::vector< Uuid> m_pending;
            std::vector< strong::resource::id> m_dynamic;
            bool m_external = false;
         };

      } // transaction
   } // common
} // casual


