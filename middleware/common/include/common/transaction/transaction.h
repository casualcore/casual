//! Copyright (c) 2015, The casual project
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT


#pragma once



#include "common/transaction/id.h"
#include "casual/platform.h"

#include "common/strong/type.h"

#include "tx.h"

#include <iosfwd>
#include <vector>
#include <chrono>

namespace casual
{
   namespace common::transaction
   {
      struct Transaction
      {

         enum class State : TRANSACTION_STATE
         {
            active = TX_ACTIVE,
            timeout = TX_TIMEOUT_ROLLBACK_ONLY,
            rollback = TX_ROLLBACK_ONLY,
         };
         std::string_view description( State value) noexcept;


         Transaction();
         explicit Transaction( ID trid);

         ID trid;
         std::optional< platform::time::point::type> deadline;
         State state = State::active;

         //! @return true if `trid` is _active_ 
         explicit operator bool() const noexcept;

         //! associated rm:s to this transaction
         inline const std::vector< strong::resource::id>& involved() const noexcept { return m_involved;}
         
         //! associate resource
         void involve( strong::resource::id id);

         template< concepts::range R>
         void involve( R&& range)
         {
            for( auto id : range) 
               involve( id);
         }  

         //! dynamic associated rm:s to this transaction, a subset to `involved`
         inline const std::vector< strong::resource::id>& dynamic() const noexcept { return m_dynamic;}
         
         //! associate id to dynamic registration
         //! @attention will not associate with involved
         //! @return true if id is not registred before
         bool associate_dynamic( strong::resource::id id);
         bool disassociate_dynamic( strong::resource::id id);

         //! associate a pending message reply
         void associate( const strong::correlation::id& correlation);

         //! discards a pending reply from this transaction
         void replied( const strong::correlation::id& correlation);

         //! @return true if this transaction has any pending replies
         //! associated
         bool pending() const noexcept;

         //! @return true if this transaction has @p correlation associated
         bool associated( const strong::correlation::id& correlation) const noexcept;

         //! associated descriptors to this transaction
         const std::vector< strong::correlation::id>& correlations() const noexcept;

         //! functions to deduce or set if the transaction is suspended
         //! or not.
         void suspend();
         void resume();
         bool suspended() const noexcept;

         //! @return true if the transaction is only local, current process 
         //!  is the owner, no pending calls, and no external involvement
         bool local() const noexcept;


         //! Associate this transaction with 'external' resources. That is,
         //! make it "not local" so it will trigger a commit request to the TM
         void external();

         friend bool operator == ( const Transaction& lhs, const ID& rhs) noexcept;
         friend bool operator == ( const Transaction& lhs, const XID& rhs) noexcept;
         inline friend bool operator == ( const Transaction& lhs, const Transaction& rhs) noexcept { return lhs.trid == rhs.trid;}
         friend bool operator == ( const Transaction& lhs, const strong::correlation::id& rhs);

         CASUAL_LOG_SERIALIZE({
            CASUAL_SERIALIZE( trid);
            CASUAL_SERIALIZE( deadline);
            CASUAL_SERIALIZE( state);
            CASUAL_SERIALIZE_NAME( m_involved, "involved");
            CASUAL_SERIALIZE_NAME( m_pending, "pending");
            CASUAL_SERIALIZE_NAME( m_dynamic, "dynamic");
            CASUAL_SERIALIZE_NAME( m_external, "external");
            CASUAL_SERIALIZE_NAME( m_suspended, "suspended");
         })

      private:
         std::vector< strong::resource::id> m_involved;
         std::vector< strong::correlation::id> m_pending;
         std::vector< strong::resource::id> m_dynamic;
         bool m_external = false;
         bool m_suspended = false;
      };

   } // common::transaction
} // casual


