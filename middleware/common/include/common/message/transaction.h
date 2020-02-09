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

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  basic_message< type>::serialize( archive);
                  CASUAL_SERIALIZE( process);
                  CASUAL_SERIALIZE( trid);
               })
            };

            template< message::Type type>
            struct basic_request : basic_transaction< type>
            {

            };

            template< typename State, message::Type type>
            struct basic_reply : basic_transaction< type>
            {
               State state = State::ok;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  basic_transaction< type>::serialize( archive);
                  CASUAL_SERIALIZE( state);
               })
            };



            namespace commit
            {
               using base_request = basic_request< Type::transaction_commit_request>;

               struct Request : base_request
               {
                  std::vector< strong::resource::id> involved;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     base_request::serialize( archive);
                     CASUAL_SERIALIZE( involved);
                  })
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

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     base_reply::serialize( archive);
                     CASUAL_SERIALIZE( stage);
                  })

                  inline friend std::ostream& operator << ( std::ostream& out, Stage value)
                  {
                     switch( value)
                     {
                        case Stage::prepare: return out << "rollback";
                        case Stage::commit: return out << "commit";
                        case Stage::error: return out << "error";
                     }
                     return out << "unknown";
                  }
               };
               static_assert( traits::is_movable< Reply>::value, "not movable");

            } // commit

            namespace rollback
            {
               using base_request = basic_request< Type::transaction_Rollback_request>;

               struct Request : base_request
               {
                  std::vector< strong::resource::id> involved;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     base_request::serialize( archive);
                     CASUAL_SERIALIZE( involved);
                  })
               };
               static_assert( traits::is_movable< Request>::value, "not movable");


               using base_reply = basic_reply< code::tx, Type::transaction_rollback_reply>;

               struct Reply : base_reply
               {
                  enum class Stage : short
                  {
                     rollback = 0,
                     error = 2,
                  };

                  Stage stage = Stage::rollback;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     base_reply::serialize( archive);
                     CASUAL_SERIALIZE( stage);
                  })

                  inline friend std::ostream& operator << ( std::ostream& out, Stage value)
                  {
                     switch( value)
                     {
                        case Stage::rollback: return out << "rollback";
                        case Stage::error: return out << "error";
                     }
                     return out << "unknown";
                  }

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
                  id::type id;
                  std::string name;
                  std::string key;

                  std::string openinfo;
                  std::string closeinfo;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     CASUAL_SERIALIZE( id);
                     CASUAL_SERIALIZE( name);
                     CASUAL_SERIALIZE( key);
                     CASUAL_SERIALIZE( openinfo);
                     CASUAL_SERIALIZE( closeinfo);
                  })
               };

               namespace lookup
               {
                  using base_request =  message::basic_request< Type::transaction_resource_lookup_request>;
                  struct Request : base_request
                  {
                     std::vector< std::string> resources;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_request::serialize( archive);
                        CASUAL_SERIALIZE( resources);
                     })
                  };
                  static_assert( traits::is_movable< Request>::value, "not movable");

                  using base_reply = message::basic_reply< Type::transaction_resource_lookup_reply>;
                  struct Reply : base_reply
                  {
                     std::vector< Resource> resources;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_reply::serialize( archive);
                        CASUAL_SERIALIZE( resources);
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

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     base_type::serialize( archive);
                     CASUAL_SERIALIZE( resource);
                     CASUAL_SERIALIZE( statistics);
                  })
               };

               namespace involved
               {  
                  using base_request = basic_transaction< Type::transaction_resource_involved_request>;
                  struct Request : base_request
                  {
                     using base_request::base_request;

                     //! potentially new resources involved
                     std::vector< strong::resource::id> involved;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_request::serialize( archive);
                        CASUAL_SERIALIZE( involved);
                     })
                  };

                  using base_reply = basic_message< Type::transaction_resource_involved_reply>;
                  struct Reply : basic_message< Type::transaction_resource_involved_reply>
                  {
                     //! resources involved prior to the request
                     std::vector< strong::resource::id> involved;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_reply::serialize( archive);
                        CASUAL_SERIALIZE( involved);
                     })
                  };
               } // involve

               template< message::Type type>
               struct basic_request : basic_transaction< type>
               {
                  using basic_transaction< type>::basic_transaction;

                  id::type resource;
                  flag::xa::Flags flags = flag::xa::Flag::no_flags;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     basic_transaction< type>::serialize( archive);
                     CASUAL_SERIALIZE( resource);
                     CASUAL_SERIALIZE( flags);
                  })
               };

               namespace configuration
               {
                  using base_request = message::basic_request< Type::transaction_resource_proxy_configuration_request>;
                  struct Request : base_request
                  {
                     using base_request::base_request;

                     id::type id;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_request::serialize( archive);
                        CASUAL_SERIALIZE( id);
                     })
                  };

                  using base_reply = message::basic_reply< Type::transaction_resource_proxy_configuration_reply>;
                  struct Reply : base_reply
                  {
                     using base_reply::base_reply;

                     resource::Resource resource;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_reply::serialize( archive);
                        CASUAL_SERIALIZE( resource);
                     })
                  };
               } // configuration


               using base_ready = message::basic_request< Type::transaction_resource_proxy_ready>;

               // sent from resource proxy when it is ready to do work
               struct Ready : base_ready
               {
                  using base_ready::base_ready;

                  id::type id;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     base_ready::serialize( archive);
                     CASUAL_SERIALIZE( id);
                  })
               };

               namespace prepare
               {
                  using Request = basic_request< Type::transaction_resource_prepare_request>;
                  using Reply = basic_reply< Type::transaction_resource_prepare_reply>;
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

               //! These request and replies are used between TM and resources when
               //! the context is of "external proxies", that is, when some other part
               //! act as a resource proxy. This semantic is used when:
               //!  * a transaction cross to another domain
               //!  * casual-queue groups enqueue and/or dequeue
               //!
               //! The resource is doing exactly the same thing but the context is
               //! preserved, so that when the TM is invoked by the reply it knows
               //! the context, and can act accordingly
               namespace external
               {

                  struct Involved : basic_transaction< Type::transaction_external_resource_involved>
                  {
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

            template<>
            struct type_traits< transaction::resource::configuration::Request> : detail::type< transaction::resource::configuration::Reply> {};

         } // reverse
      } // message
   } // common
} // casual


