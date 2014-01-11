


#ifndef CASUAL_MESSAGES_H_
#define CASUAL_MESSAGES_H_




#include "common/ipc.h"
#include "common/buffer_context.h"
//#include "common/types.h"
#include "common/platform.h"
#include "common/process.h"
#include "common/exception.h"
#include "common/uuid.h"
#include "common/transaction_id.h"

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
            cTransactionManagerConnect = 100,
            cTransactionManagerConfiguration,
            cTransactionManagerReady,
            cTransactionBeginRequest,
            cTransactionBeginReply,
            cTransactionCommitRequest,
            cTransactionCommitReply,
            cTransactionRollbackRequest,
            cTransactionRollbackReply,
            cTransactionGenericReply,
            //cTransactionPrepareReply,
            cTransactionResurceConnectReply,
            //cTransactionResurceGenericReply,
            cTransactionResourcePrepareRequest,
            cTransactionResourcePrepareReply,
            cTransactionResourceCommitRequest,
            cTransactionResourceCommitReply,
            cTransactionResourceRollbackRequest,
            cTransactionResourceRollbackReply,
            cTransactionDomainResourcePrepareRequest,
            cTransactionDomainResourcePrepareReply,
            cTransactionDomainResourceCommitRequest,
            cTransactionDomainResourceCommitReply,
            cTransactionDomainResourceRollbackRequest,
            cTransactionDomainResourceRollbackReply,
            cTransactionResourceInvolved,

         };


         template< message::Type type>
         struct basic_messsage
         {
            enum
            {
               message_type = type
            };
         };


         struct Transaction
         {
            typedef common::platform::pid_type pid_type;

            transaction::ID xid;
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

            explicit Service( const std::string& name_) : name( name_)
            {}

            std::string name;
            Seconds timeout = 0;
            common::platform::queue_id_type monitor_queue = 0;
            bool auto_transaction = false;

            template< typename A>
            void marshal( A& archive)
            {
               archive & name;
               archive & timeout;
               archive & monitor_queue;
               archive & auto_transaction;
            }
         };

         namespace resource
         {
            struct Manager
            {
               std::size_t instances = 0;
               std::size_t id = 0;
               std::string key;
               std::string openinfo;
               std::string closeinfo;

               template< typename A>
               void marshal( A& archive)
               {
                  archive & instances;
                  archive & id;
                  archive & key;
                  archive & openinfo;
                  archive & closeinfo;
               }
            };

         } // resource


         namespace server
         {

            //!
            //! Represents id for a server.
            //!
            struct Id
            {

               typedef platform::queue_id_type queue_id_type;
               typedef common::platform::pid_type pid_type;


               Id() = default;
               Id( queue_id_type id, pid_type pid) : queue_id( id), pid( pid) {}


               queue_id_type queue_id = 0;
               pid_type pid = common::process::id();

               template< typename A>
               void marshal( A& archive)
               {
                  archive & queue_id;
                  archive & pid;
               }
            };

            inline std::ostream& operator << ( std::ostream& out, const Id& value)
            {
               return out << "pid: " << value.pid << " queue: " << value.queue_id;
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
            struct Configuration : basic_messsage< cServerConfiguration>
            {

               Configuration() = default;
               Configuration( Configuration&&) = default;
               Configuration& operator = ( Configuration&&) = default;

               typedef platform::queue_id_type queue_id_type;

               queue_id_type transactionManagerQueue = 0;
               std::vector< resource::Manager> resourceManagers;
               std::string domain;

               template< typename A>
               void marshal( A& archive)
               {
                  archive & transactionManagerQueue;
                  archive & resourceManagers;
                  archive & domain;
               }
            };

            typedef basic_disconnect< cServerDisconnect> Disconnect;

         } // server



         namespace service
         {

            struct Advertise : basic_messsage< cServiceAdvertise>
            {

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

            struct Unadvertise : basic_messsage< cServiceUnadvertise>
            {
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
                  struct Request : basic_messsage< cServiceNameLookupRequest>
                  {

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
                  struct Reply : basic_messsage< cServiceNameLookupReply>
                  {

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

            struct base_call : basic_messsage< cServiceCall>
            {

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
               Transaction transaction;

               template< typename A>
               void marshal( A& archive)
               {
                  archive & callDescriptor;
                  archive & service;
                  archive & reply;
                  archive & callId;
                  archive & callee;
                  archive & transaction;
               }
            };

            namespace callee
            {

               //!
               //! Represents a service call. via tp(a)call, from the callee's perspective
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
               //!
               //! Represents a service call. via tp(a)call, from the callers perspective
               //!
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
            struct Reply :  basic_messsage< cServiceReply>
            {

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
            struct ACK : basic_messsage< cServiceAcknowledge>
            {

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
            struct Notify : basic_messsage< cMonitorNotify>
            {

               std::string parentService;
               std::string service;

               common::Uuid callId;

               std::string transactionId;

               common::platform::time_type start;
               common::platform::time_type end;

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
            //! Used to connect the transaction manager to broker
            //!
            typedef server::basic_connect< cTransactionManagerConnect> Connect;


            struct Configuration : message::basic_messsage< cTransactionManagerConfiguration>
            {
               std::string domain;
               std::vector< message::resource::Manager> resources;

               template< typename A>
               void marshal( A& archive)
               {
                  archive & domain;
                  archive & resources;
               }
            };


            struct Connected : message::basic_messsage< cTransactionManagerReady>
            {
               bool success = true;

               template< typename A>
               void marshal( A& archive)
               {
                  archive & success;
               }
            };


            template< message::Type type>
            struct basic_transaction : basic_messsage< type>
            {
               typedef basic_transaction< type> base_type;

               server::Id id;
               common::transaction::ID xid;

               template< typename A>
               void marshal( A& archive)
               {
                  archive & id;
                  archive & xid;
               }
            };




            template< message::Type type>
            struct basic_request : basic_transaction< type>
            {
               typedef basic_transaction< type> base_type;

            };

            template< message::Type type>
            struct basic_reply : basic_transaction< type>
            {
               typedef basic_transaction< type> base_type;

               platform::resource::id_type resource = 0;
               int state = 0;

               template< typename A>
               void marshal( A& archive)
               {
                  base_type::marshal( archive);
                  archive & resource;
                  archive & state;
               }
            };

            namespace begin
            {
               struct Request : public basic_request< cTransactionBeginRequest>
               {
                  template< typename A>
                  void marshal( A& archive)
                  {
                     base_type::marshal( archive);
                     archive & start;
                  }

                  common::platform::time_type start;
               };

               typedef basic_reply< cTransactionBeginReply> Reply;

            } // begin


            namespace commit
            {
               typedef basic_request< cTransactionCommitRequest> Request;
               typedef basic_reply< cTransactionCommitReply> Reply;
            } // commit

            namespace rollback
            {
               typedef basic_request< cTransactionRollbackRequest> Request;
               typedef basic_reply< cTransactionRollbackReply> Reply;
            } // rollback


            namespace resource
            {
               struct Involved : basic_transaction< cTransactionResourceInvolved>
               {
                  std::vector< platform::resource::id_type> resources;

                  template< typename A>
                  void marshal( A& archive)
                  {
                     base_type::marshal( archive);
                     archive & resources;
                  }
               };

               template< message::Type type>
               struct basic_request : basic_transaction< type>
               {
                  typedef basic_transaction< type> base_type;

                  int flags = 0;

                  template< typename A>
                  void marshal( A& archive)
                  {
                     base_type::marshal( archive);
                     archive & flags;
                  }

               };

               namespace connect
               {
                  //!
                  //! Used to notify the TM that a resource proxy is up and running, or not...
                  //!
                  struct Reply : basic_messsage< cTransactionResurceConnectReply>
                  {
                     server::Id id;
                     platform::resource::id_type resource = 0;
                     int state = 0;

                     template< typename A>
                     void marshal( A& archive)
                     {
                        archive & id;
                        archive & resource;
                        archive & state;
                     }
                  };
               } // connect

               namespace prepare
               {
                  typedef basic_request< cTransactionResourcePrepareRequest> Request;
                  typedef basic_reply< cTransactionResourcePrepareReply> Reply;

               } // prepare

               namespace commit
               {
                  typedef basic_request< cTransactionResourceCommitRequest> Request;
                  typedef basic_reply< cTransactionResourceCommitReply> Reply;

               } // commit

               namespace rollback
               {
                  typedef basic_request< cTransactionResourceRollbackRequest> Request;
                  typedef basic_reply< cTransactionResourceRollbackReply> Reply;

               } // rollback


               //!
               //! These request and replies are used between TM and resources when
               //! the context is of "inter-domain", that is, when TM is acting as
               //! a resource to other domains.
               //! The resource is doing exactly the same thing but the context is
               //! preserved, so that when the TM is invoked by the reply it knows
               //! the context, and can act accordingly
               //!
               namespace domain
               {
                  namespace prepare
                  {
                     typedef basic_request< cTransactionDomainResourcePrepareRequest> Request;
                     typedef basic_reply< cTransactionDomainResourcePrepareReply> Reply;

                  } // prepare

                  namespace commit
                  {
                     typedef basic_request< cTransactionDomainResourceCommitRequest> Request;
                     typedef basic_reply< cTransactionDomainResourceCommitReply> Reply;

                  } // commit

                  namespace rollback
                  {
                     typedef basic_request< cTransactionDomainResourceRollbackRequest> Request;
                     typedef basic_reply< cTransactionDomainResourceRollbackReply> Reply;

                  } // rollback
               } // domain



            } // resource


         } // transaction

         //!
         //! Deduce witch type of message it is.
         //!
         template< typename M>
         ipc::message::Transport::message_type_type type( const M&)
         {
            return M::message_type;
         }
      } // message
   } //common
} // casual

#endif /* CASUAL_IPC_MESSAGES_H_ */
