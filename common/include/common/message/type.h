//!
//! type.h
//!
//! Created on: Jun 14, 2014
//!     Author: Lazan
//!

#ifndef COMMONMESSAGETYPE_H_
#define COMMONMESSAGETYPE_H_

#include "common/platform.h"
#include "common/transaction/id.h"

#include "common/marshal.h"


namespace casual
{
   namespace common
   {
      namespace message
      {
         enum Type
         {

            UTILITY_BASE = 500,
            cTerminate,

            // Server
            SERVER_BASE = 1000, // message type can't be 0!
            cServerConnectRequest,
            cServerConnectReply,
            cServerDisconnect,
            cServerPingRequest,
            cServerPingReply,

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
            cQueueConnectRequest,
            cQueueConnectReply,
            cQueueEnqueueRequest,
            cQueueDequeueRequest,
            cQueueDequeueReply,
            cQueueInformation,
            cQueueQueuesInformationRequest,
            cQueueQueuesInformationReply,
            cQueueQueueInformationRequest,
            cQueueQueueInformationReply,
            cQueueLookupRequest,
            cQueueLookupReply,
            cQueueGroupInvolved,
         };

         template< message::Type type>
         struct basic_message
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
         constexpr platform::message_type_type type( const M&)
         {
            return M::message_type;
         }


         //!
         //! Message to "force" exit/termination.
         //! useful in unittest, to force exit on blocking read
         //!
         using Terminate = basic_message< cTerminate>;


         //
         // Below, some basic message related types that is used by others
         //

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

            template< message::Type type>
            struct basic_id : basic_message< type>
            {

               process::Handle process;

               template< typename A>
               void marshal( A& archive)
               {
                  archive & process;
               }
            };


            template< message::Type type>
            struct basic_connect : basic_id< type>
            {

               std::string path;

               template< typename A>
               void marshal( A& archive)
               {
                  basic_id< type>::marshal( archive);
                  archive & path;
               }
            };

            template< message::Type type>
            struct basic_disconnect : basic_id< type>
            {

            };

         } // server
      } // message
   } // common
} // casual

#endif // TYPE_H_
