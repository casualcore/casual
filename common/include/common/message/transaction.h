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

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     archive & instances;
                     archive & id;
                     archive & key;
                     archive & openinfo;
                     archive & closeinfo;
                  })
               };

            } // resource

            namespace client
            {
               namespace connect
               {

                  using Request = server::connect::basic_request< cTransactionClientConnectRequest>;

                  //!
                  //! Sent from the broker with "transaction-information" for a server/client
                  //!
                  struct Reply : server::connect::basic_reply< cTransactionClientConnectReply>
                  {

                     using queue_id_type = platform::queue_id_type;

                     queue_id_type transaction_manager = 0;
                     std::vector< resource::Manager> resources;
                     std::string domain;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        server::connect::basic_reply< cTransactionClientConnectReply>::marshal( archive);
                        archive & transaction_manager;
                        archive & resources;
                        archive & domain;
                     })
                  };
               } // connect

            } // client

            namespace manager
            {
               //!
               //! Used to connect the transaction manager to broker
               //!
               namespace connect
               {
                  using Request = server::connect::basic_request< cTransactionManagerConnectRequest>;
                  using Reply = server::connect::basic_reply< cTransactionManagerConnectReply>;

               } // connect



               struct Configuration : message::basic_message< cTransactionManagerConfiguration>
               {
                  std::string domain;
                  std::vector< resource::Manager> resources;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     base_type::marshal( archive);
                     archive & domain;
                     archive & resources;
                  })
               };


               struct Ready : message::basic_message< cTransactionManagerReady>
               {
                  process::Handle process;
                  bool success = true;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     base_type::marshal( archive);
                     archive & process;
                     archive & success;
                  })
               };
            } // manager


            template< message::Type type>
            struct basic_transaction : basic_message< type>
            {
               using base_type = basic_transaction< type>;

               process::Handle process;
               common::transaction::ID trid;

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  basic_message< type>::marshal( archive);
                  archive & process;
                  archive & trid;
               })
            };




            template< message::Type type>
            struct basic_request : basic_transaction< type>
            {

            };

            template< message::Type type>
            struct basic_reply : basic_transaction< type>
            {
               int state = 0;

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  basic_transaction< type>::marshal( archive);
                  archive & state;
               })
            };

            namespace begin
            {
               struct Request : public basic_request< cTransactionBeginRequest>
               {
                  common::platform::time_point start;
                  std::chrono::microseconds timeout;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     base_type::marshal( archive);
                     archive & start;
                     archive & timeout;
                  })
               };

               struct Reply : basic_reply< cTransactionBeginReply>
               {
                  enum class Stage : char
                  {
                     begin = 0,
                     error = 2,
                  };

                  Stage stage = Stage::begin;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     basic_reply< cTransactionBeginReply>::marshal( archive);
                     archive & stage;
                  })
               };

            } // begin


            namespace commit
            {
               typedef basic_request< cTransactionCommitRequest> Request;

               struct Reply : basic_reply< cTransactionCommitReply>
               {
                  enum class Stage : char
                  {
                     prepare = 0,
                     commit = 1,
                     error = 2,
                  };

                  Stage stage = Stage::prepare;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     basic_reply< cTransactionCommitReply>::marshal( archive);
                     archive & stage;
                  })

               };

            } // commit

            namespace rollback
            {
               typedef basic_request< cTransactionRollbackRequest> Request;

               struct Reply : basic_reply< cTransactionRollbackReply>
               {
                  enum class Stage : char
                  {
                     rollback = 0,
                     error = 2,
                  };

                  Stage stage = Stage::rollback;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     basic_reply< cTransactionRollbackReply>::marshal( archive);
                     archive & stage;
                  })
               };
            } // rollback


            namespace resource
            {

               template< message::Type type>
               struct basic_reply : transaction::basic_reply< type>
               {
                  platform::resource::id_type resource = 0;
                  Statistics statistics;


                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     transaction::basic_reply< type>::marshal( archive);
                     archive & resource;
                     archive & statistics;
                  })
               };

               struct Involved : basic_transaction< cTransactionResourceInvolved>
               {
                  std::vector< platform::resource::id_type> resources;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     base_type::marshal( archive);
                     archive & resources;
                  })

                  friend std::ostream& operator << ( std::ostream& out, const Involved& value);

               };

               template< message::Type type>
               struct basic_request : basic_transaction< type>
               {
                  using base_type = basic_request;

                  platform::resource::id_type resource = 0;
                  int flags = 0;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     basic_transaction< type>::marshal( archive);
                     archive & resource;
                     archive & flags;
                  })

               };

               namespace connect
               {
                  //!
                  //! Used to notify the TM that a resource proxy is up and running, or not...
                  //!
                  struct Reply : basic_message< cTransactionResurceConnectReply>
                  {
                     process::Handle process;
                     platform::resource::id_type resource = 0;
                     int state = 0;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_type::marshal( archive);
                        archive & process;
                        archive & resource;
                        archive & state;
                     })

                     friend std::ostream& operator << ( std::ostream& out, const Reply& message);
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

         namespace reverse
         {

            template<>
            struct type_traits< transaction::manager::connect::Request> : detail::type< transaction::manager::connect::Reply> {};
            template<>
            struct type_traits< transaction::client::connect::Request> : detail::type< transaction::client::connect::Reply> {};


            template<>
            struct type_traits< transaction::begin::Request> : detail::type< transaction::begin::Reply> {};
            template<>
            struct type_traits< transaction::commit::Request> : detail::type< transaction::commit::Reply> {};
            template<>
            struct type_traits< transaction::rollback::Request> : detail::type< transaction::rollback::Reply> {};

            template<>
            struct type_traits< transaction::resource::prepare::Request> : detail::type< transaction::resource::prepare::Reply> {};
            template<>
            struct type_traits< transaction::resource::commit::Request> : detail::type< transaction::resource::commit::Reply> {};
            template<>
            struct type_traits< transaction::resource::rollback::Request> : detail::type< transaction::resource::rollback::Reply> {};

            template<>
            struct type_traits< transaction::resource::domain::prepare::Request> : detail::type< transaction::resource::domain::prepare::Reply> {};
            template<>
            struct type_traits< transaction::resource::domain::commit::Request> : detail::type< transaction::resource::domain::commit::Reply> {};
            template<>
            struct type_traits< transaction::resource::domain::rollback::Request> : detail::type< transaction::resource::domain::rollback::Reply> {};

         } // reverse


      } // message
   } // common
} // casual

#endif // TRANSACTION_H_
