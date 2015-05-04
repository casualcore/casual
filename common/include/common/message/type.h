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

#include "common/marshal/marshal.h"


namespace casual
{
   namespace common
   {
      namespace message
      {
         enum Type
         {

            UTILITY_BASE = 500,
            cFlushIPC, // dummy message used to flush queue (into cache)
            cShutdowndRequest,
            cShutdowndReply,

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
            TRAFFICMONITOR_BASE = 3000,
            cTrafficMonitorConnect,
            cTrafficMonitorDisconnect,
            cTrafficMonitorNotify,

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
            cQueueEnqueueRequest = 5100,
            cQueueEnqueueReply,
            cQueueDequeueRequest = 5200,
            cQueueDequeueReply,
            cQueueDequeueCallbackRequest,
            cQueueDequeueCallbackReply,
            cQueueInformation = 5300,
            cQueueQueuesInformationRequest,
            cQueueQueuesInformationReply,
            cQueueQueueInformationRequest,
            cQueueQueueInformationReply,
            cQueueLookupRequest = 5400,
            cQueueLookupReply,
            cQueueGroupInvolved = 5500,
         };

         template< message::Type type>
         struct basic_message
         {
            enum
            {
               message_type = type
            };

            Uuid correlation;
         };

         template< typename M, message::Type type>
         struct type_wrapper : public M
         {
            using M::M;

            enum
            {
               message_type = type
            };

            Uuid correlation;
         };

         namespace flush
         {
            struct IPC : basic_message< cFlushIPC>
            {
               char dummy;
               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  archive & dummy;
               })
            };
         } // flush


         //!
         //! Deduce witch type of message it is.
         //!
         template< typename M>
         constexpr platform::message_type_type type( const M&)
         {
            return M::message_type;
         }

         template< typename M>
         auto correlation( M& message) -> decltype( message.correlation)
         {
            return message.correlation;
         }


         //!
         //! Message to "force" exit/termination.
         //! useful in unittest, to force exit on blocking read
         //!
         namespace shutdown
         {
            struct Request : basic_message< cShutdowndRequest>
            {
               process::Handle process;
               bool reply = false;

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  archive & reply;
               })
            };

            struct Reply : basic_message< cShutdowndReply>
            {
               template< typename ID>
               struct holder_t
               {
                  std::vector< ID> online;
                  std::vector< ID> offline;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     archive & online;
                     archive & offline;
                  })
               };

               holder_t< platform::pid_type> executables;
               holder_t< process::Handle> servers;


               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  archive & executables;
                  archive & servers;
               })
            };

         } // shutdown



         //
         // Below, some basic message related types that is used by others
         //

         struct Service
         {

            Service() = default;
            Service& operator = (const Service& rhs) = default;



            explicit Service( std::string name, std::uint64_t type, std::uint64_t transaction)
               : name( std::move( name)), type( type), transaction( transaction)
            {}

            explicit Service( std::string name)
               : Service( std::move( name), 0, 0)
            {}

            std::string name;
            std::uint64_t type = 0;
            std::chrono::microseconds timeout = std::chrono::microseconds::zero();
            std::vector< platform::queue_id_type> traffic_monitors;
            std::uint64_t transaction = 0;

            CASUAL_CONST_CORRECT_MARSHAL(
            {
               archive & name;
               archive & type;
               archive & timeout;
               archive & traffic_monitors;
               archive & transaction;
            })

            friend std::ostream& operator << ( std::ostream& out, const Service& value);
         };

         namespace server
         {

            template< message::Type type>
            struct basic_id : basic_message< type>
            {

               process::Handle process;

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  archive & process;
               })
            };


            template< message::Type type>
            struct basic_connect : basic_id< type>
            {

               std::string path;

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  basic_id< type>::marshal( archive);
                  archive & path;
               })
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
