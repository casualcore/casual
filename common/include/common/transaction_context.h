//!
//! context.h
//!
//! Created on: Jul 14, 2013
//!     Author: Lazan
//!

#ifndef CONTEXT_H_
#define CONTEXT_H_

#include <tx.h>

#include "common/ipc.h"
#include "common/message.h"
#include "common/transaction_id.h"

#include <stack>

namespace casual
{
   namespace common
   {
      namespace transaction
      {

         void unique_xid( XID& xid);


         const char* xaError( int code);

         const char* txError( int code);

         struct Resource
         {
            Resource( const char* key, xa_switch_t* xa) : key( key), xaSwitch( xa), id( nextResurceId()) {}

            const std::string key;
            xa_switch_t* xaSwitch;
            const int id;

            std::string openinfo;
            std::string closeinfo;

            static int nextResurceId();
         };

         struct Transaction
         {


            // using state_type = TRANSACTION_STATE;
            typedef TRANSACTION_STATE state_type;
            enum class State : state_type
            {
               active = TX_ACTIVE,
               rollback = TX_ROLLBACK_ONLY,
               timeout = TX_TIMEOUT_ROLLBACK_ONLY,
               inactive,
            };

            //typedef TRANSACTION_TIMEOUT Seconds;
            //Seconds timeout = 0;

            ID xid;
            common::platform::pid_type owner = 0;
            State state = State::inactive;
            bool suspended = false;
         };

         struct State
         {
            typedef TRANSACTION_CONTROL control_type;
            enum class Control : control_type
            {
               unchained = TX_UNCHAINED,
               chained = TX_CHAINED,
            };

            //!
            //! @return transaction manager queue
            //!
            ipc::send::Queue::id_type manager();

            //ipc::send::Queue::id_type transactionManagerQueue = 0;

            std::vector< Resource> resources;

            std::stack< Transaction> transactions;

            Control control = Control::unchained;



         };

         class Context
         {
         public:

            static Context& instance();

            //!
            //! Correspond to the tx API
            //!
            //! @{
            void open();
            void close();

            int begin();
            int commit();
            int rollback();

            void setCommitReturn( COMMIT_RETURN value);
            void setTransactionControl(TRANSACTION_CONTROL control);
            void setTransactionTimeout(TRANSACTION_TIMEOUT timeout);
            void info( TXINFO& info);
            //! @}

            //!
            //! Correspond to the ax API
            //!
            //! @{
            int resourceRegistration( int rmid, XID* xid, long flags);
            int resourceUnregistration( int rmid, long flags);
            //! @}



            void apply( const message::server::Configuration& configuration);

            //!
            //! Associate ongoing transaction, or start a new one if XID is null
            //!
            void associateOrStart( const message::Transaction& transaction);


            //!
            //! commits or rollback transaction created from this server
            //!
            void finalize( const message::service::Reply& message);


            //!
            //! @return current transaction. 'null xid' if there are none...
            //!
            const Transaction& currentTransaction() const;

            State& state();

         private:

            Context();

            State m_state;

         };

      } // transaction
   } // common
} // casual



#endif /* CONTEXT_H_ */
