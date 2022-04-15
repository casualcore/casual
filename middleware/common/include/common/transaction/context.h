//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include <tx.h>


#include "common/transaction/id.h"
#include "common/transaction/resource.h"
#include "common/transaction/transaction.h"

#include "common/message/transaction.h"
#include "common/message/service.h"

#include "common/code/tx.h"
#include "common/flag/xa.h"


#include <stack>

namespace casual
{
   namespace common::transaction
   {
      enum class Control : TRANSACTION_CONTROL
      {
         unchained = TX_UNCHAINED,
         chained = TX_CHAINED,
         stacked = TX_STACKED
      };
      std::string_view description( Control value) noexcept;

      namespace commit
      {
         enum class Return : COMMIT_RETURN
         {
            completed = TX_COMMIT_COMPLETED,
            logged = TX_COMMIT_DECISION_LOGGED
         };
         std::string_view description( Return value) noexcept;

      } // commit

      struct Context
      {
         static Context& instance();

         //! Correspond to the tx API
         //! @{
         void open();
         void close();

         void begin();
         void commit();
         void rollback();

         void set_commit_return( commit::Return value) noexcept;
         commit::Return get_commit_return() const noexcept;
         
         void set_transaction_control( transaction::Control control);
         void set_transaction_timeout( platform::time::unit timeout);
         
         bool info( TXINFO* info);
         //! @}

         //! Correspond to casual extension of the tx API
         //! @{
         void suspend( XID* xid);
         void resume( const XID* xid);
         //! @}

         //! Correspond to the ax API
         //! @{
         code::ax resource_registration( strong::resource::id rmid, XID* xid);
         void resource_unregistration( strong::resource::id rmid);
         //! @}

         //! @ingroup service-start
         //!
         //! Join transaction. could be a null xid.
         Transaction& join( const transaction::ID& trid);

         //! @ingroup service-start
         //!
         //! Start a new transaction
         Transaction& start( const platform::time::point::type& start);

         //! @ingroup service-start
         //!
         //! branch transaction, if null-xid, we start a new one
         Transaction& branch( const transaction::ID& trid);

         //! trid server is invoked with
         //! @{
         transaction::ID caller;

         void update( message::service::call::Reply& state);

         //! commits or rollback transaction created from this server
         message::service::Transaction finalize( bool commit);

         //! @return current transaction. 'null xid' if there are none...
         Transaction& current();

         //! @return true if @p correlation is associated with an active transaction
         bool associated( const strong::correlation::id& correlation);

         void configure( std::vector< resource::Link> resources);

         //! @return true if there are pending transactions that is owned by this
         //! process
         bool pending() const;

         std::vector< strong::resource::id> resources() const;

         //! @attention only for unittest... temporary until we fix the "context-fiasco".
         static Context& clear();

         //! return true if the context holds no transactions, only (?) for unittest
         bool empty() const;
         
         void resources_suspend( Transaction& transaction);
         void resources_resume( Transaction& transaction);

      private:

         std::vector< Transaction> m_transactions;

         //! 'null' transaction, used when no transaction present...
         Transaction m_empty;

         Control m_control = Control::unchained;
         commit::Return m_commit_return = commit::Return::completed;

         struct resources_type
         {
            std::vector< Resource> all;

            using range_type = common::range::type_t< std::vector< Resource>>;
            range_type fixed;
            range_type dynamic;

         } m_resources;


         platform::time::unit m_timeout{};

         Context();
         ~Context();

         void commit( const Transaction& transaction);
         void rollback( const Transaction& transaction);


         void resource_commit( strong::resource::id rm, const Transaction& transaction, flag::xa::Flags flags);
         void resource_rollback( strong::resource::id rm, const Transaction& transaction);

         void pop_transaction();


      };

      inline Context& context() { return Context::instance();}

   } // common::transaction
} // casual




