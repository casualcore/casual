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

            template< message::Type type>
            struct basic_transaction : basic_message< type>
            {
               using base_type = basic_transaction< type>;

               common::process::Handle process;
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



            namespace commit
            {
               struct Request : basic_request< Type::transaction_commit_request>
               {
                  std::vector< platform::resource::id::type> resources;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     basic_request< Type::transaction_commit_request>::marshal( archive);
                     archive & resources;
                  })
               };

               struct Reply : basic_reply< Type::transaction_commit_reply>
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
                     basic_reply< Type::transaction_commit_reply>::marshal( archive);
                     archive & stage;
                  })

               };

            } // commit

            namespace rollback
            {
               struct Request : basic_request< Type::transaction_Rollback_request>
               {
                  std::vector< platform::resource::id::type> resources;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     basic_request< Type::transaction_Rollback_request>::marshal( archive);
                     archive & resources;
                  })
               };

               struct Reply : basic_reply< Type::transaction_rollback_reply>
               {
                  enum class Stage : char
                  {
                     rollback = 0,
                     error = 2,
                  };

                  Stage stage = Stage::rollback;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     basic_reply< Type::transaction_rollback_reply>::marshal( archive);
                     archive & stage;
                  })
               };
            } // rollback


            namespace resource
            {

               template< message::Type type>
               struct basic_reply : transaction::basic_reply< type>
               {
                  platform::resource::id::type resource = 0;
                  Statistics statistics;


                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     transaction::basic_reply< type>::marshal( archive);
                     archive & resource;
                     archive & statistics;
                  })

                  friend std::ostream& operator << ( std::ostream& out, const basic_reply& message)
                  {
                     return out << "{ trid: " << message.trid
                           << ", process: " << message.process
                           << ", resource: " << message.resource
                           << ", state: " << message.state
                           << ", statistics: " << message.statistics
                           << '}';
                  }
               };

               struct Involved : basic_transaction< Type::transaction_resource_involved>
               {
                  std::vector< platform::resource::id::type> resources;

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

                  platform::resource::id::type resource = 0;
                  int flags = 0;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     basic_transaction< type>::marshal( archive);
                     archive & resource;
                     archive & flags;
                  })

                  friend std::ostream& operator << ( std::ostream& out, const basic_request& message)
                  {
                     return out << "{ trid: " << message.trid
                           << ", process: " << message.process
                           << ", resurce: " << message.resource
                           << ", flags: " << message.flags
                           << '}';
                  }

               };

               namespace connect
               {
                  //!
                  //! Used to notify the TM that a resource proxy is up and running, or not...
                  //!
                  struct Reply : basic_message< Type::transaction_resurce_connect_reply>
                  {
                     common::process::Handle process;
                     platform::resource::id::type resource = 0;
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
                  typedef basic_request< Type::transaction_resource_prepare_request> Request;
                  typedef basic_reply< Type::transaction_resource_prepare_reply> Reply;

               } // prepare

               namespace commit
               {
                  typedef basic_request< Type::transaction_resource_commit_request> Request;
                  typedef basic_reply< Type::transaction_resource_commit_reply> Reply;

               } // commit

               namespace rollback
               {
                  typedef basic_request< Type::transaction_resource_rollback_request> Request;
                  typedef basic_reply< Type::transaction_resource_rollback_reply> Reply;

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

                  struct Involved : basic_transaction< Type::transaction_domain_resource_involved>
                  {
                     Uuid domain;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_type::marshal( archive);
                        archive & domain;
                     })

                     friend std::ostream& operator << ( std::ostream& out, const Involved& value);
                  };

                  namespace involved
                  {
                     template< typename M>
                     Involved create( const Uuid& domain, M&& message)
                     {
                        Involved involved;

                        involved.domain = domain;
                        involved.correlation = message.correlation;
                        involved.execution = message.execution;
                        involved.process = common::process::handle();
                        involved.trid = message.trid;

                        return involved;
                     }
                  } // involved

                  /*
                  namespace prepare
                  {
                     typedef basic_request< Type::transaction_domain_resource_prepare_request> Request;
                     typedef basic_reply< Type::transaction_domain_resource_prepare_reply> Reply;

                  } // prepare

                  namespace commit
                  {
                     typedef basic_request< Type::transaction_domain_resource_commit_request> Request;
                     typedef basic_reply< Type::transaction_domain_resource_commit_reply> Reply;

                  } // commit

                  namespace rollback
                  {
                     typedef basic_request< Type::transaction_domain_resource_rollback_request> Request;
                     typedef basic_reply< Type::transaction_domain_resource_rollback_reply> Reply;

                  } // rollback
                  */
               } // domain



            } // resource


         } // transaction

         namespace reverse
         {

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

            /*
            template<>
            struct type_traits< transaction::resource::domain::prepare::Request> : detail::type< transaction::resource::domain::prepare::Reply> {};
            template<>
            struct type_traits< transaction::resource::domain::commit::Request> : detail::type< transaction::resource::domain::commit::Reply> {};
            template<>
            struct type_traits< transaction::resource::domain::rollback::Request> : detail::type< transaction::resource::domain::rollback::Reply> {};
            */

         } // reverse


      } // message
   } // common
} // casual

#endif // TRANSACTION_H_
