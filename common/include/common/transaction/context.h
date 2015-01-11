//!
//! context.h
//!
//! Created on: Jul 14, 2013
//!     Author: Lazan
//!

#ifndef CASUAL_COMMON_TRANSACTION_CONTEXT_H_
#define CASUAL_COMMON_TRANSACTION_CONTEXT_H_

#include <tx.h>


#include "common/transaction/id.h"
#include "common/transaction/resource.h"
#include "common/transaction/transaction.h"

#include "common/ipc.h"
#include "common/message/transaction.h"
#include "common/message/server.h"


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

            //!
            //! Correspond to the tx API
            //!
            //! @{
            int open();
            void close();

            int begin();
            int commit();
            int rollback();



            int setCommitReturn( COMMIT_RETURN value);
            int setTransactionControl(TRANSACTION_CONTROL control);
            int setTransactionTimeout(TRANSACTION_TIMEOUT timeout);
            int info( TXINFO& info);
            //! @}

            //!
            //! Correspond to the ax API
            //!
            //! @{
            int resourceRegistration( int rmid, XID* xid, long flags);
            int resourceUnregistration( int rmid, long flags);
            //! @}

            int suspend();
            int resume();

            //!
            //! Associate ongoing transaction, or start a new one if XID is null
            //!
            void joinOrStart( const transaction::ID& transaction);


            void update( message::service::Reply& state);

            //!
            //! commits or rollback transaction created from this server
            //!
            void finalize( message::service::Reply& message);


            //!
            //! @return current transaction. 'null xid' if there are none...
            //!
            Transaction& current();


            //!
            //! @return true if @p descriptor is associated with an active transaction
            //!
            bool associated( platform::descriptor_type descriptor);


            void set( const std::vector< Resource>& resources);


         private:

            typedef TRANSACTION_CONTROL control_type;
            enum class Control : control_type
            {
               unchained = TX_UNCHAINED,
               chained = TX_CHAINED,
            };

            Control control = Control::unchained;


            struct resources_type
            {
               std::vector< Resource> all;

               using range_type = typename common::range::traits< std::vector< Resource>>::type;
               range_type fixed;
               range_type dynamic;

            } m_resources;


            std::vector< Transaction> m_transactions;

            //!
            //! Resources outside any global transaction
            //! (this could only be dynamic rm:s)
            //!
            std::vector< int> m_outside;

            //!
            //! Attributes that is initialized from "manager"
            //!
            struct Manager
            {
               static const Manager& instance();

               ipc::send::Queue::id_type queue = 0;
               std::vector< message::transaction::resource::Manager> resources;
            private:
               Manager();
            };

            const Manager& manager();

            void involved( transaction::ID& xid, std::vector< int> resources);

            void apply( const message::transaction::client::connect::Reply& configuration);

            Context();


            int commit( const Transaction& transaction);
            int rollback( const Transaction& transaction);


            void start( Transaction& transaction, long flags);
            void end( const Transaction& transaction, long flags);



         };

      } // transaction
   } // common
} // casual



#endif /* CONTEXT_H_ */
