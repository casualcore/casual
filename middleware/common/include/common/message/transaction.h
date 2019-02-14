//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/message/type.h"

#include "common/strong/id.h"
#include "common/code/xa.h"
#include "common/code/tx.h"
#include "common/flag/xa.h"


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

               inline friend std::ostream& operator << ( std::ostream& out, const basic_transaction& message)
               {
                  return out << "{ process: " << message.process
                        << ", trid: " << message.trid
                        << '}';
               }

            };




            template< message::Type type>
            struct basic_request : basic_transaction< type>
            {

            };

            template< typename State, message::Type type>
            struct basic_reply : basic_transaction< type>
            {
               State state = State::ok;

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  basic_transaction< type>::marshal( archive);
                  archive & state;
               })
            };



            namespace commit
            {
               using base_request = basic_request< Type::transaction_commit_request>;

               struct Request : base_request
               {
                  std::vector< strong::resource::id> involved;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                        base_request::marshal( archive);
                        archive & involved;
                  })

                  friend std::ostream& operator << ( std::ostream& out, const Request& message);
               };
               static_assert( traits::is_movable< Request>::value, "not movable");

               using base_reply = basic_reply< code::tx, Type::transaction_commit_reply>;
               struct Reply : base_reply
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
                     base_reply::marshal( archive);
                     archive & stage;
                  })

                  friend std::ostream& operator << ( std::ostream& out, const Stage& stage);
                  friend std::ostream& operator << ( std::ostream& out, const Reply& message);
               };
               static_assert( traits::is_movable< Reply>::value, "not movable");

            } // commit

            namespace rollback
            {
               using base_request = basic_request< Type::transaction_Rollback_request>;

               struct Request : base_request
               {
                  std::vector< strong::resource::id> involved;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     base_request::marshal( archive);
                     archive & involved;
                  })

                  friend std::ostream& operator << ( std::ostream& out, const Request& message);
               };
               static_assert( traits::is_movable< Request>::value, "not movable");


               using base_reply = basic_reply< code::tx, Type::transaction_rollback_reply>;

               struct Reply : base_reply
               {
                  enum class Stage : char
                  {
                     rollback = 0,
                     error = 2,
                  };

                  Stage stage = Stage::rollback;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     base_reply::marshal( archive);
                     archive & stage;
                  })
                  friend std::ostream& operator << ( std::ostream& out, const Reply::Stage& message);
                  friend std::ostream& operator << ( std::ostream& out, const Reply& message);
               };
               static_assert( traits::is_movable< Reply>::value, "not movable");
            } // rollback


            namespace resource
            {
               namespace id
               {
                  using type = strong::resource::id;
               } // id

               struct Resource
               {
                  Resource() = default;
                  Resource( std::function< void(Resource&)> foreign) { foreign( *this);}

                  id::type id;
                  std::string name;
                  std::string key;

                  std::string openinfo;
                  std::string closeinfo;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     archive & id;
                     archive & name;
                     archive & key;
                     archive & openinfo;
                     archive & closeinfo;
                  })
               };

               namespace lookup
               {
                  using base_request =  message::basic_request< Type::transaction_resource_lookup_request>;
                  struct Request : base_request
                  {
                     std::vector< std::string> resources;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_request::marshal( archive);
                        archive & resources;
                     })
                  };
                  static_assert( traits::is_movable< Request>::value, "not movable");

                  using base_reply = message::basic_reply< Type::transaction_resource_lookup_reply>;
                  struct Reply : base_reply
                  {
                     std::vector< Resource> resources;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_reply::marshal( archive);
                        archive & resources;
                     })
                  };
                  static_assert( traits::is_movable< Reply>::value, "not movable");

               } // lookup

               template< message::Type type>
               struct basic_reply : transaction::basic_reply< code::xa, type>
               {
                  using base_type = transaction::basic_reply< code::xa, type>;
                  id::type resource;
                  Statistics statistics;


                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     base_type::marshal( archive);
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

               namespace involved
               {  
                  struct Request : basic_transaction< Type::transaction_resource_involved_request>
                  {
                     //! potentially new resorces involved
                     std::vector< strong::resource::id> involved;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_type::marshal( archive);
                        archive & involved;
                     })

                     friend std::ostream& operator << ( std::ostream& out, const Request& value);
                  };

                  struct Reply : basic_message< Type::transaction_resource_involved_reply>
                  {
                     //! resources involved prior to the request
                     std::vector< strong::resource::id> involved;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_type::marshal( archive);
                        archive & involved;
                     })

                     friend std::ostream& operator << ( std::ostream& out, const Reply& value);
                  };
               } // involve


               template< message::Type type>
               struct basic_request : basic_transaction< type>
               {
                  using base_type = basic_request;

                  id::type resource;
                  flag::xa::Flags flags = flag::xa::Flag::no_flags;

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
                           << ", resource: " << message.resource
                           << ", flags: " << message.flags
                           << '}';
                  }
               };

               namespace connect
               {
                  //!
                  //! Used to notify the TM that a resource proxy is up and running, or not...
                  //!
                  struct Reply : basic_message< Type::transaction_resource_connect_reply>
                  {
                     common::process::Handle process;
                     id::type resource;
                     code::xa state = code::xa::ok;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_type::marshal( archive);
                        archive & process;
                        archive & resource;
                        archive & state;
                     })

                     friend std::ostream& operator << ( std::ostream& out, const Reply& message);
                  };
                  static_assert( traits::is_movable< Reply>::value, "not movable");
               } // connect

               namespace prepare
               {
                  using Request = basic_request< Type::transaction_resource_prepare_request>;
                  using Reply = basic_reply< Type::transaction_resource_prepare_reply>;

                  static_assert( traits::is_movable< Request>::value, "not movable");
                  static_assert( traits::is_movable< Reply>::value, "not movable");

               } // prepare

               namespace commit
               {
                  using Request = basic_request< Type::transaction_resource_commit_request>;
                  using Reply = basic_reply< Type::transaction_resource_commit_reply>;

               } // commit

               namespace rollback
               {
                  using Request = basic_request< Type::transaction_resource_rollback_request>;
                  using Reply = basic_reply< Type::transaction_resource_rollback_reply>;

               } // rollback


               //!
               //! These request and replies are used between TM and resources when
               //! the context is of "external proxies", that is, when some other part
               //! act as a resource proxy. This semantic is used when:
               //!  * a transaction cross to another domain
               //!  * casual-queue groups enqueue and/or dequeue
               //!
               //! The resource is doing exactly the same thing but the context is
               //! preserved, so that when the TM is invoked by the reply it knows
               //! the context, and can act accordingly
               //!
               namespace external
               {

                  struct Involved : basic_transaction< Type::transaction_external_resource_involved>
                  {

                     friend std::ostream& operator << ( std::ostream& out, const Involved& value);
                  };
                  static_assert( traits::is_movable< Involved>::value, "not movable");

                  namespace involved
                  {
                     template< typename M>
                     Involved create( M&& message)
                     {
                        Involved involved;

                        involved.correlation = message.correlation;
                        involved.execution = message.execution;
                        involved.process = common::process::handle();
                        involved.trid = message.trid;

                        return involved;
                     }
                  } // involved
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
            struct type_traits< transaction::resource::involved::Request> : detail::type< transaction::resource::involved::Reply> {};

            template<>
            struct type_traits< transaction::resource::lookup::Request> : detail::type< transaction::resource::lookup::Reply> {};

            template<>
            struct type_traits< transaction::resource::prepare::Request> : detail::type< transaction::resource::prepare::Reply> {};
            template<>
            struct type_traits< transaction::resource::commit::Request> : detail::type< transaction::resource::commit::Reply> {};
            template<>
            struct type_traits< transaction::resource::rollback::Request> : detail::type< transaction::resource::rollback::Reply> {};


         } // reverse
      } // message
   } // common
} // casual


