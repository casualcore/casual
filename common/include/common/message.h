//!
//! casual_ipc_messages.h
//!
//! Created on: Apr 25, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_MESSAGES_H_
#define CASUAL_MESSAGES_H_

#include "common/ipc.h"
#include "common/buffer_context.h"
#include "common/types.h"
#include "common/platform.h"
#include "common/exception.h"
#include "common/uuid.h"

#include <vector>
#include <chrono>

namespace casual
{
   namespace common
   {

      namespace message
      {
         enum Type
         {

            cServerConnect  = 10, // message type can't be 0!
            cServerConfiguration,
            cServerDisconnect,
            cServiceAdvertise = 20,
            cServiceUnadvertise,
            cServiceNameLookupRequest,
            cServiceNameLookupReply,
            cServiceCall,
            cServiceReply,
            cServiceAcknowledge,
            cMonitorConnect = 30,
            cMonitorDisconnect,
            cMonitorNotify,
            cTransactionManagerConnect = 40,
            cTransactionBegin,
            cTransactionCommit,
            cTransactionRollback,
            cTransactionReply

            //cTransactionMonitorUnadvertise,

         };



         struct Service
         {
            typedef int Seconds;

            Service() = default;
            Service& operator = (const Service& rhs) = default;

            explicit Service( const std::string& name_) : name( name_)
            {}

            std::string name;
            Seconds timeout = 0;
            common::platform::queue_key_type monitor_queue = 0;

            template< typename A>
            void marshal( A& archive)
            {
               archive & name;
               archive & timeout;
               archive & monitor_queue;
            }
         };

         namespace server
         {

            //!
            //! Represents id for a server.
            //!
            struct Id
            {
               typedef common::platform::pid_type pid_type;
               typedef platform::queue_key_type queue_key_type;

               Id()
                     : pid( common::platform::getProcessId())
               {
               }

               queue_key_type queue_key;
               pid_type pid;

               template< typename A>
               void marshal( A& archive)
               {
                  archive & queue_key;
                  archive & pid;
               }
            };

            template< message::Type type>
            struct basic_connect
            {
               enum
               {
                  message_type = type
               };

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
            struct basic_disconnect
            {
               enum
               {
                  message_type = type
               };

               server::Id server;

               template< typename A>
               void marshal( A& archive)
               {
                  archive & server;
               }
            };

            struct Connect : public basic_connect< cServerConnect>
            {
               typedef basic_connect< cServerConnect> base_type;

               std::vector< Service> services;

               template< typename A>
               void marshal( A& archive)
               {
                  base_type::marshal( archive);
                  archive & services;
               }

            };

            //!
            //! Sent from the broker with "start-up-information" for a server
            //!
            struct Configuration
            {
               enum
               {
                  message_type = cServerConfiguration
               };

               typedef platform::queue_key_type queue_key_type;

               queue_key_type transactionManagerQueue = 0;

               template< typename A>
               void marshal( A& archive)
               {
                  archive & transactionManagerQueue;
               }
            };

            typedef basic_disconnect< cServerDisconnect> Disconnect;

         } // server



         namespace service
         {

            struct Advertise
            {
               enum
               {
                  message_type = cServiceAdvertise
               };

               std::string serverPath;
               server::Id server;
               std::vector< Service> services;

               template< typename A>
               void marshal( A& archive)
               {
                  archive & serverPath;
                  archive & server;
                  archive & services;
               }
            };

            struct Unadvertise
            {
               enum
               {
                  message_type = cServiceUnadvertise
               };

               server::Id server;
               std::vector< Service> services;

               template< typename A>
               void marshal( A& archive)
               {
                  archive & server;
                  archive & services;
               }
            };

            namespace name
            {
               namespace lookup
               {
                  //!
                  //! Represent "service-name-lookup" request.
                  //!
                  struct Request
                  {
                     enum
                     {
                        message_type = cServiceNameLookupRequest
                     };

                     std::string requested;
                     server::Id server;

                     template< typename A>
                     void marshal( A& archive)
                     {
                        archive & requested;
                        archive & server;
                     }
                  };

                  //!
                  //! Represent "service-name-lookup" response.
                  //!
                  struct Reply
                  {

                     enum
                     {
                        message_type = cServiceNameLookupReply
                     };

                     Service service;

                     std::vector< server::Id> server;

                     template< typename A>
                     void marshal( A& archive)
                     {
                        archive & service;
                        archive & server;
                     }
                  };
               } // lookup
            } // name

            struct base_call
            {
               enum
               {
                  message_type = cServiceCall
               };

               base_call() = default;

               base_call( base_call&&) = default;
               base_call& operator = ( base_call&&) = default;

               base_call( const base_call&) = delete;
               base_call& operator = ( const base_call&) = delete;

               int callDescriptor = 0;
               Service service;
               server::Id reply;
               common::Uuid callId;
               std::string callee;

               template< typename A>
               void marshal( A& archive)
               {
                  archive & callDescriptor;
                  archive & service;
                  archive & reply;
                  archive & callId;
                  archive & callee;
               }
            };

            namespace callee
            {

               //!
               //! Represents a service call. via tp(a)call
               //!
               struct Call: public base_call
               {

                  Call() = default;

                  Call( Call&&) = default;
                  Call& operator = ( Call&&) = default;

                  Call( const Call&) = delete;
                  Call& operator = ( const Call&) = delete;

                  buffer::Buffer buffer;

                  template< typename A>
                  void marshal( A& archive)
                  {
                     base_call::marshal( archive);
                     archive & buffer;
                  }
               };}

            namespace caller
            {
               struct Call: public base_call
               {

                  Call( buffer::Buffer& buffer_)
                        : buffer( buffer_)
                  {
                  }

                  Call( Call&&) = default;
                  Call& operator = ( Call&&) = default;

                  Call( const Call&) = delete;
                  Call& operator = ( const Call&) = delete;

                  buffer::Buffer& buffer;

                  template< typename A>
                  void marshal( A& archive)
                  {
                     base_call::marshal( archive);
                     archive & buffer;
                  }
               };

            }

            //!
            //! Represent service reply.
            //!
            struct Reply
            {
               enum
               {
                  message_type = cServiceReply
               };

               Reply() = default;
               Reply( Reply&&) = default;


               Reply( const Reply&) = delete;
               Reply& operator = ( const Reply&) = delete;

               int callDescriptor = 0;
               int returnValue = 0;
               long userReturnCode = 0;
               buffer::Buffer buffer;

               template< typename A>
               void marshal( A& archive)
               {
                  archive & callDescriptor;
                  archive & returnValue;
                  archive & userReturnCode;
                  archive & buffer;
               }

            };

            //!
            //! Represent the reply to the broker when a server is done handling
            //! a service-call and is ready for new calls
            //!
            struct ACK
            {
               enum
               {
                  message_type = cServiceAcknowledge
               };

               std::string service;
               server::Id server;

               template< typename A>
               void marshal( A& archive)
               {
                  archive & service;
                  archive & server;
               }
            };
         } // service


         namespace monitor
         {
            //!
            //! Used to advertise the monitorserver
            //!
            typedef server::basic_connect< cMonitorConnect> Connect;

            //!
            //! Used to unadvertise the monitorserver
            //!
            typedef server::basic_disconnect< cMonitorDisconnect> Disconnect;

            //!
            //! Notify monitorserver with statistics
            //!
            struct Notify
            {
               enum
               {
                  message_type = cMonitorNotify
               };

               std::string parentService;
               std::string service;

               common::Uuid callId;

               std::string transactionId;

               common::time_type start;
               common::time_type end;

               template< typename A>
               void marshal( A& archive)
               {
                  archive & parentService;
                  archive & service;
                  archive & callId;
                  archive & transactionId;
                  archive & start;
                  archive & end;
               }
            };
         } // monitor

         namespace transaction
         {
            //!
            //! Used to connect the transaction monitor to broker
            //!
            typedef server::basic_connect< cTransactionManagerConnect> Connect;

            //!
            //! Used to unadvertise the transaction monitor
            //!
            //typedef basic_disconnect< cTransactionMonitorUnadvertise> Unadvertise;

            template< message::Type type>
            struct basic_transaction
            {
               enum
               {
                  message_type = type
               };

               server::Id server;
               XID xid;

               template< typename A>
               void marshal( A& archive)
               {
                  archive & server;
                  archive & xid;
               }
            };

            typedef basic_transaction< cTransactionBegin> Begin;
            typedef basic_transaction< cTransactionCommit> Commit;
            typedef basic_transaction< cTransactionRollback> Rollback;

            struct Reply
            {
               enum
               {
                  message_type = cTransactionReply
               };

               int state;

               template< typename A>
               void marshal( A& archive)
               {
                  archive & state;
               }

            };


            /*
             * extern int tx_begin(void);
   extern int tx_close(void);
   extern int tx_commit(void);
   extern int tx_open(void);
   extern int tx_rollback(void);

      cTransactionBegin,
            cTransactionCommit,
            cTransactionRollback,
             */





         }

         //!
         //! Deduce witch type of message it is.
         //!
         template< typename M>
         ipc::message::Transport::message_type_type type( const M& message)
         {
            return M::message_type;
         }
      } // message
   } //common
} // casual

#endif /* CASUAL_IPC_MESSAGES_H_ */
