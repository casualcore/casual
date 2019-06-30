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
   namespace common
   {
      namespace transaction
      {

         class Context
         {
         public:

            static Context& instance();

            //! Correspond to the tx API
            //! @{
            void open();
            void close();

            void begin();
            void commit();
            void rollback();



            void set_commit_return( COMMIT_RETURN value);
            COMMIT_RETURN get_commit_return();
            
            void set_transaction_control( TRANSACTION_CONTROL control);
            void set_transaction_timeout( TRANSACTION_TIMEOUT timeout);
            
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
            void join( const transaction::ID& trid);

            //! @ingroup service-start
            //!
            //! Start a new transaction
            void start( const platform::time::point::type& start);

            //! @ingroup service-start
            //!
            //! branch transaction, if null-xid, we start a new one
            void branch( const transaction::ID& trid);

            //! trid server is invoked with
            //! @{
            transaction::ID caller;

            void update( message::service::call::Reply& state);

            //! commits or rollback transaction created from this server
            message::service::Transaction finalize( bool commit);

            //! @return current transaction. 'null xid' if there are none...
            Transaction& current();

            //! @return true if @p correlation is associated with an active transaction
            bool associated( const Uuid& correlation);

            void configure( std::vector< resource::Link> resources, std::vector< std::string> names);

            //! @return true if there are pending transactions that is owned by this
            //! process
            bool pending() const;

         private:

            using control_type = TRANSACTION_CONTROL;
            enum class Control : control_type
            {
               unchained = TX_UNCHAINED,
               chained = TX_CHAINED,
               stacked = TX_STACKED
            };

            Control m_control = Control::unchained;

            using commit_return_type = COMMIT_RETURN;

            // TODO: change name
            enum class Commit_Return : commit_return_type
            {
               completed = TX_COMMIT_COMPLETED,
               logged = TX_COMMIT_DECISION_LOGGED
            };

            Commit_Return m_commit_return = Commit_Return::completed;


            struct resources_type
            {
               std::vector< Resource> all;

               using range_type = common::range::type_t< std::vector< Resource>>;
               range_type fixed;
               range_type dynamic;

            } m_resources;

            std::vector< strong::resource::id> resources() const;


            std::vector< Transaction> m_transactions;

            transaction::ID m_caller;

            TRANSACTION_TIMEOUT m_timeout = 0;

            Context();

            void commit( const Transaction& transaction);
            void rollback( const Transaction& transaction);

            void resources_start( Transaction& transaction, flag::xa::Flags flags);
            void resources_end( const Transaction& transaction, flag::xa::Flags flags);
            void resource_commit( strong::resource::id rm, const Transaction& transaction, flag::xa::Flags flags);
            void resource_rollback( strong::resource::id rm, const Transaction& transaction);

            void pop_transaction();


         };

         inline Context& context() { return Context::instance();}
      } // transaction
   } // common
} // casual




