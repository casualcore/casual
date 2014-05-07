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

            explicit operator bool() const { return ! xid.null() && ! suspended;}

            //typedef TRANSACTION_TIMEOUT Seconds;
            //Seconds timeout = 0;

            ID xid;
            common::platform::pid_type owner = 0;
            State state = State::inactive;
            bool suspended = false;
         };

         inline std::ostream& operator << ( std::ostream& out, const Transaction& rhs)
         {
            return out << "{xid: " << rhs.xid << ", owner: " << rhs.owner << ", state: " << rhs.state << ", suspended: " << rhs.suspended << "}";
         }

         struct Resource
         {
            Resource( const char* key, xa_switch_t* xa) : key( key), xaSwitch( xa) {}


            int commmit( const Transaction& transaction, long flags);
            int rollback( const Transaction& transaction, long flags);

            int start( const Transaction& transaction, long flags);
            int end( const Transaction& transaction, long flags);

            int open( long flags);
            int close( long flags);


            const std::string key;
            xa_switch_t* xaSwitch;
            int id = 0;

            std::string openinfo;
            std::string closeinfo;

         };

         inline std::ostream& operator << ( std::ostream& out, const Resource& resource)
         {
            return out << "{key: " << resource.key << ", id: " << resource.id << ", openinfo: " << resource.openinfo << ", closeinfo: " << resource.closeinfo << "}";
         }

         class Context;



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


            //!
            //! Associate ongoing transaction, or start a new one if XID is null
            //!
            void associateOrStart( const message::Transaction& transaction);


            //!
            //! commits or rollback transaction created from this server
            //!
            void finalize( message::service::Reply& message);


            //!
            //! @return current transaction. 'null xid' if there are none...
            //!
            const Transaction& currentTransaction() const;


            void set( const std::vector< Resource>& resources);


         private:

            typedef TRANSACTION_CONTROL control_type;
            enum class Control : control_type
            {
               unchained = TX_UNCHAINED,
               chained = TX_CHAINED,
            };

            Control control = Control::unchained;


            std::vector< Resource> m_resources;
            std::stack< Transaction> m_transactions;

            //!
            //! Attributes that is initialized from "manager"
            //!
            struct Manager
            {
               static const Manager& instance();

               ipc::send::Queue::id_type queue = 0;
               std::vector< message::resource::Manager> resources;
            private:
               Manager();
            };

            const Manager& manager();

            void apply( const message::transaction::client::connect::Reply& configuration);

            Context();


            int commit( const Transaction& transaction);
            int rollback( const Transaction& transaction);


            void start( const Transaction& transaction, long flags);
            void end( const Transaction& transaction, long flags);



         };

      } // transaction
   } // common
} // casual



#endif /* CONTEXT_H_ */
