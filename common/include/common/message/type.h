//!
//! type.h
//!
//! Created on: Jun 14, 2014
//!     Author: Lazan
//!

#ifndef COMMONMESSAGETYPE_H_
#define COMMONMESSAGETYPE_H_

#include "common/platform.h"
#include "common/transaction_id.h"


namespace casual
{
   namespace common
   {
      namespace message
      {
         enum Type
         {

            // Server
            SERVER_BASE = 1000, // message type can't be 0!
            cServerConnectRequest,
            cServerConnectReply,
            cServerDisconnect,

            // Service
            SERVICE_BASE = 2000,
            cServiceAdvertise,
            cServiceUnadvertise,
            cServiceNameLookupRequest,
            cServiceNameLookupReply,
            cServiceCall,
            cServiceReply,
            cServiceAcknowledge,

            // Monitor
            MONITOR_BASE = 3000,
            cMonitorConnect,
            cMonitorDisconnect,
            cMonitorNotify,

            // Transaction
            TRANSACTION_BASE = 4000,
            cTransactionClientConnectRequest,
            cTransactionClientConnectReply,
            cTransactionManagerConnect,
            cTransactionManagerConfiguration,
            cTransactionManagerReady,
            cTransactionBeginRequest = 4100,
            cTransactionBeginReply,
            cTransactionCommitRequest,
            cTransactionPrepareReply,
            cTransactionCommitReply,
            cTransactionRollbackRequest,
            cTransactionRollbackReply,
            cTransactionGenericReply,

            cTransactionResurceConnectReply = 4200,
            cTransactionResourcePrepareRequest,
            cTransactionResourcePrepareReply,
            cTransactionResourceCommitRequest,
            cTransactionResourceCommitReply,
            cTransactionResourceRollbackRequest,
            cTransactionResourceRollbackReply,
            cTransactionDomainResourcePrepareRequest = 4300,
            cTransactionDomainResourcePrepareReply,
            cTransactionDomainResourceCommitRequest,
            cTransactionDomainResourceCommitReply,
            cTransactionDomainResourceRollbackRequest,
            cTransactionDomainResourceRollbackReply,
            cTransactionResourceInvolved = 4400,

            // casual queue
            QUEUE_BASE = 5000,
            cQueueEnqueueRequest,
            cQueueDequeueRequest,
            cQueueDequeueReply,
            cQueueInformation,
         };

         template< message::Type type>
         struct basic_messsage
         {
            enum
            {
               message_type = type
            };
         };


         //!
         //! Deduce witch type of message it is.
         //!
         template< typename M>
         platform::message_type_type type( const M&)
         {
            return M::message_type;
         }



         //
         // Below, some basic message related types that is used by others
         //

         struct Transaction
         {
            typedef common::platform::pid_type pid_type;

            common::transaction::ID xid;
            pid_type creator = 0;

            template< typename A>
            void marshal( A& archive)
            {
               archive & xid;
               archive & creator;
            }
         };


         struct Service
         {
            typedef int Seconds;

            Service() = default;
            Service& operator = (const Service& rhs) = default;

            explicit Service( const std::string& name)
               : name( name)
            {}

            explicit Service( const std::string& name, long type, int transaction)
               : name( name), type( type), transaction( transaction)
            {}

            std::string name;
            long type = 0;
            Seconds timeout = 0;
            common::platform::queue_id_type monitor_queue = 0;
            int transaction = 0;

            template< typename A>
            void marshal( A& archive)
            {
               archive & name;
               archive & type;
               archive & timeout;
               archive & monitor_queue;
               archive & transaction;
            }
         };

         namespace server
         {

            //!
            //! Represents id for a server.
            //!
            struct Id
            {

               typedef platform::queue_id_type queue_id_type;
               typedef common::platform::pid_type pid_type;


               Id();
               Id( queue_id_type id, pid_type pid);
               Id( Id&&) = default;
               Id& operator = ( Id&&) = default;

               Id( const Id&) = default;
               Id& operator = ( const Id&) = default;


               static Id current();


               queue_id_type queue_id = 0;
               pid_type pid;

               template< typename A>
               void marshal( A& archive)
               {
                  archive & queue_id;
                  archive & pid;
               }
            };
            inline std::ostream& operator << ( std::ostream& out, const Id& value)
            {
               return out << "{pid: " << value.pid << " queue: " << value.queue_id << "}";
            }


            template< message::Type type>
            struct basic_connect : basic_messsage< type>
            {
               server::Id server;
               std::string path;

               template< typename A>
               void marshal( A& archive)
               {
                  archive & server;
                  archive & path;
               }
            };

            template< message::Type type>
            struct basic_disconnect : basic_messsage< type>
            {
               server::Id server;

               template< typename A>
               void marshal( A& archive)
               {
                  archive & server;
               }
            };

         } // server
      } // message
   } // common
} // casual

#endif // TYPE_H_
