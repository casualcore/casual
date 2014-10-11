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
#include "common/message/transaction.h"
#include "common/message/server.h"
#include "common/transaction_id.h"

#include <stack>

namespace casual
{
   namespace common
   {
      namespace transaction
      {

         struct Resource;

         struct Transaction
         {

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

            explicit operator bool() const { return ! xid.null() && m_state != State::suspended;}

            //typedef TRANSACTION_TIMEOUT Seconds;
            //Seconds timeout = 0;

            ID xid;
            common::platform::pid_type owner = 0;

            //!
            //! associated rm:s to this transaction
            //!
            std::vector< int> associated;


            friend std::ostream& operator << ( std::ostream& out, const Transaction& rhs)
            {
               return out << "{xid: " << rhs.xid << ", owner: " << rhs.owner << ", state: " << rhs.m_state << ", previous: " << rhs.m_previous << "}";
            }

         private:
            State m_state = State::inactive;
            State m_previous = State::inactive;
         };


         struct Resource
         {
            Resource( std::string key, xa_switch_t* xa, int id, std::string openinfo, std::string closeinfo)
               : key( std::move( key)), xaSwitch( xa), id( id), openinfo( std::move( openinfo)), closeinfo( std::move( closeinfo)) {}

            Resource( std::string key, xa_switch_t* xa) : Resource( std::move( key), xa, 0, std::string(), std::string()) {}




            int start( const Transaction& transaction, long flags);
            int end( const Transaction& transaction, long flags);

            int open( long flags);
            int close( long flags);


            std::string key;
            xa_switch_t* xaSwitch;
            int id = 0;

            std::string openinfo;
            std::string closeinfo;

            bool dynamic() const { return xaSwitch->flags & TMREGISTER;}

            friend std::ostream& operator << ( std::ostream& out, const Resource& resource)
            {
               return out << "{key: " << resource.key << ", id: " << resource.id << ", openinfo: " << resource.openinfo << ", closeinfo: " << resource.closeinfo << "}";
            }
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
            void joinOrStart( const message::Transaction& transaction);


            //!
            //! commits or rollback transaction created from this server
            //!
            void finalize( message::service::Reply& message);


            //!
            //! @return current transaction. 'null xid' if there are none...
            //!
            Transaction& currentTransaction();


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

            //std::vector< Resource> m_resources;
            std::stack< Transaction> m_transactions;

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
