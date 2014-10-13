//!
//! transaction.h
//!
//! Created on: Jun 14, 2014
//!     Author: Lazan
//!

#ifndef TRANSACTION_H_
#define TRANSACTION_H_

#include "common/message/type.h"


namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace transaction
         {
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

            namespace client
            {
               namespace connect
               {

                  typedef server::basic_connect< cTransactionClientConnectRequest> Request;

                  //!
                  //! Sent from the broker with "transaction-information" for a server/client
                  //!
                  struct Reply : basic_message< cTransactionClientConnectReply>
                  {

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
               } // connect

            } // client

            namespace manager
            {
               //!
               //! Used to connect the transaction manager to broker
               //!
               typedef server::basic_connect< cTransactionManagerConnect> Connect;


               struct Configuration : message::basic_message< cTransactionManagerConfiguration>
               {
                  std::string domain;
                  std::vector< resource::Manager> resources;

                  template< typename A>
                  void marshal( A& archive)
                  {
                     archive & domain;
                     archive & resources;
                  }
               };


               struct Ready : message::basic_message< cTransactionManagerReady>
               {
                  server::Id id;
                  bool success = true;

                  template< typename A>
                  void marshal( A& archive)
                  {
                     archive & id;
                     archive & success;
                  }
               };
            } // manager


            template< message::Type type>
            struct basic_transaction : basic_message< type>
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

            namespace prepare
            {
               typedef basic_reply< cTransactionPrepareReply> Reply;
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

                  platform::resource::id_type resource = 0;
                  int flags = 0;

                  template< typename A>
                  void marshal( A& archive)
                  {
                     base_type::marshal( archive);
                     archive & resource;
                     archive & flags;
                  }

               };

               namespace connect
               {
                  //!
                  //! Used to notify the TM that a resource proxy is up and running, or not...
                  //!
                  struct Reply : basic_message< cTransactionResurceConnectReply>
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


      } // message
   } // common
} // casual

#endif // TRANSACTION_H_
