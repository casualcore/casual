//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "common/message/type.h"
#include "common/transaction/id.h"
#include "common/transaction/global.h"

#include "common/strong/id.h"
#include "common/code/xa.h"
#include "common/code/tx.h"
#include "common/flag/xa.h"
#include "common/process.h"


namespace casual
{
   namespace common::message::transaction
   {

      template< message::Type type>
      struct basic_transaction : basic_request< type>
      {
         using basic_request< type>::basic_request;

         common::transaction::ID trid;

         CASUAL_CONST_CORRECT_SERIALIZE(
            basic_request< type>::serialize( archive);
            CASUAL_SERIALIZE( trid);
         )
      };

      template< message::Type type>
      using basic_request = basic_transaction< type>;


      template< typename State, message::Type type>
      struct basic_reply : basic_transaction< type>
      {
         using basic_transaction< type>::basic_transaction;

         State state = State::ok;

         CASUAL_CONST_CORRECT_SERIALIZE(
            basic_transaction< type>::serialize( archive);
            CASUAL_SERIALIZE( state);
         )
      };


      namespace commit
      {
         using base_request = basic_request< Type::transaction_commit_request>;
         struct Request : base_request
         {
            using base_request::base_request;

            std::vector< strong::resource::id> involved;

            CASUAL_CONST_CORRECT_SERIALIZE(
               base_request::serialize( archive);
               CASUAL_SERIALIZE( involved);
            )
         };

         namespace reply
         {
            enum class Stage : std::int8_t
            {
               prepare = 0,
               commit = 1,
               rollback = 2,
            };

            inline constexpr std::string_view description( Stage value)
            {
               switch( value)
               {
                  case Stage::prepare: return "prepare";
                  case Stage::commit: return "commit";
                  case Stage::rollback: return "rollback";
               }
               return "<unknown>";
            }

            using base = basic_reply< code::tx, Type::transaction_commit_reply>;
            
         } // reply

         
         struct Reply : reply::base
         {
            reply::Stage stage = reply::Stage::prepare;

            CASUAL_CONST_CORRECT_SERIALIZE(
               reply::base::serialize( archive);
               CASUAL_SERIALIZE( stage);
            )
         };

      } // commit

      namespace rollback
      {
         using base_request = basic_request< Type::transaction_rollback_request>;
         struct Request : base_request
         {
            using base_request::base_request;
            
            std::vector< strong::resource::id> involved;

            CASUAL_CONST_CORRECT_SERIALIZE(
               base_request::serialize( archive);
               CASUAL_SERIALIZE( involved);
            )
         };


         using base_reply = basic_reply< code::tx, Type::transaction_rollback_reply>;
         struct Reply : base_reply
         {
            enum class Stage : short
            {
               rollback = 0,
               error = 2,
            };
            inline constexpr std::string_view description( Stage value)
            {
               switch( value)
               {
                  case Stage::rollback: return "rollback";
                  case Stage::error: return "error";
               }
               return "unknown";
            }

            Stage stage = Stage::rollback;

            CASUAL_CONST_CORRECT_SERIALIZE(
               base_reply::serialize( archive);
               CASUAL_SERIALIZE( stage);
            )
         };

      } // rollback

      namespace configuration
      {
         struct Resource
         {
            strong::resource::id id;
            std::string name;
            std::string key;
            std::string openinfo;
            std::string closeinfo;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( id);
               CASUAL_SERIALIZE( name);
               CASUAL_SERIALIZE( key);
               CASUAL_SERIALIZE( openinfo);
               CASUAL_SERIALIZE( closeinfo);
            )
         };

         namespace alias
         {
            using base_request = basic_request< Type::transaction_configuration_alias_request>;
            struct Request : base_request
            {
               using base_request::base_request;
               
               std::string alias;
               //! named resources
               std::vector< std::string> resources;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_request::serialize( archive);
                  CASUAL_SERIALIZE( alias);
                  CASUAL_SERIALIZE( resources);
               )                     
            };

            using base_reply = basic_message< Type::transaction_configuration_alias_reply>;
            struct Reply : base_reply
            {
               using base_reply::base_reply;

               std::vector< Resource> resources;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_reply::serialize( archive);
                  CASUAL_SERIALIZE( resources);
               ) 
            };
         } // alias
         
      } // configuration


      namespace resource
      {
         namespace id
         {
            using type = strong::resource::id;
         } // id

         template< message::Type type>
         struct basic_reply : transaction::basic_reply< code::xa, type>
         {
            using base_type = transaction::basic_reply< code::xa, type>;
            using base_type::base_type;
            id::type resource;
            Statistics statistics;

            CASUAL_CONST_CORRECT_SERIALIZE(
               base_type::serialize( archive);
               CASUAL_SERIALIZE( resource);
               CASUAL_SERIALIZE( statistics);
            )
         };

         namespace involved
         {  
            using base_request = basic_transaction< Type::transaction_resource_involved_request>;
            struct Request : base_request
            {
               using base_request::base_request;

               //! potentially new resources involved
               std::vector< strong::resource::id> involved;
               bool reply = true;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_request::serialize( archive);
                  CASUAL_SERIALIZE( involved);
                  CASUAL_SERIALIZE( reply);
               )
            };

            using base_reply = basic_message< Type::transaction_resource_involved_reply>;
            struct Reply : base_reply
            {
               //! resources involved prior to the request
               std::vector< strong::resource::id> involved;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_reply::serialize( archive);
                  CASUAL_SERIALIZE( involved);
               )
            };
         } // involve

         template< message::Type type>
         struct basic_request : basic_transaction< type>
         {
            using basic_transaction< type>::basic_transaction;

            id::type resource;
            flag::xa::Flag flags = flag::xa::Flag::no_flags;

            CASUAL_CONST_CORRECT_SERIALIZE(
               basic_transaction< type>::serialize( archive);
               CASUAL_SERIALIZE( resource);
               CASUAL_SERIALIZE( flags);
            )
         };

         namespace configuration
         {
            using base_request = message::basic_request< Type::transaction_resource_proxy_configuration_request>;
            struct Request : base_request
            {
               using base_request::base_request;

               id::type id;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_request::serialize( archive);
                  CASUAL_SERIALIZE( id);
               )
            };

            using base_reply = message::basic_reply< Type::transaction_resource_proxy_configuration_reply>;
            struct Reply : base_reply
            {
               using base_reply::base_reply;

               transaction::configuration::Resource resource;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_reply::serialize( archive);
                  CASUAL_SERIALIZE( resource);
               )
            };
         } // configuration


         using base_ready = message::basic_request< Type::transaction_resource_proxy_ready>;

         // sent from resource proxy when it is ready to do work
         struct Ready : base_ready
         {
            using base_ready::base_ready;

            id::type id;

            CASUAL_CONST_CORRECT_SERIALIZE(
               base_ready::serialize( archive);
               CASUAL_SERIALIZE( id);
            )
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
            using base_instance = message::basic_process< Type::transaction_external_resource_instance>;

            //! Message that is sent by external resources to register
            //! them self to TM. This allows extra information valuable to 
            //! users. 
            struct Instance : base_instance
            {
               using base_instance::base_instance;

               // the alias of the executable
               std::string alias;
               // human readable information (ex. domain-name)
               std::string description;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_instance::serialize( archive);
                  CASUAL_SERIALIZE( alias);
                  CASUAL_SERIALIZE( description);
               )

            };

            using Involved = basic_transaction< Type::transaction_external_resource_involved>;

            namespace involved
            {
               template< typename M>
               Involved create( M&& message, const common::process::Handle& handle = common::process::handle())
               {
                  Involved involved;

                  involved.correlation = message.correlation;
                  involved.execution = message.execution;
                  involved.process = handle;
                  involved.trid = message.trid;

                  return involved;
               }
            } // involved

         } // external
      } // resource

      namespace inbound::branch
      {
         using base_request = message::basic_request< message::Type::transaction_inbound_branch_request>;
         struct Request : base_request
         {
            using base_request::base_request;

            common::transaction::global::ID gtrid;

            CASUAL_CONST_CORRECT_SERIALIZE(
               base_request::serialize( archive);
               CASUAL_SERIALIZE( gtrid);
            )
         };

         using Reply = transaction::basic_request< message::Type::transaction_inbound_branch_reply>;

      } // inbound::branch

      namespace active
      {
         template< typename Base>
         struct basic_active : Base
         {
            using Base::Base;
            std::vector< common::transaction::global::ID> gtrids;

            CASUAL_CONST_CORRECT_SERIALIZE(
               Base::serialize( archive);
               CASUAL_SERIALIZE( gtrids);
            )
         };

         using Request = basic_active< message::basic_request< message::Type::transaction_active_request>>;
         using Reply = basic_active< message::basic_reply< message::Type::transaction_active_reply>>;         
      } // active

   } // common::message::transaction

   namespace common::message::reverse
   {
      template<>
      struct type_traits< transaction::commit::Request> : detail::type< transaction::commit::Reply> {};
      template<>
      struct type_traits< transaction::rollback::Request> : detail::type< transaction::rollback::Reply> {};

      template<>
      struct type_traits< transaction::configuration::alias::Request> : detail::type< transaction::configuration::alias::Reply> {};

      template<>
      struct type_traits< transaction::resource::involved::Request> : detail::type< transaction::resource::involved::Reply> {};

      template<>
      struct type_traits< transaction::resource::prepare::Request> : detail::type< transaction::resource::prepare::Reply> {};
      template<>
      struct type_traits< transaction::resource::commit::Request> : detail::type< transaction::resource::commit::Reply> {};
      template<>
      struct type_traits< transaction::resource::rollback::Request> : detail::type< transaction::resource::rollback::Reply> {};

      template<>
      struct type_traits< transaction::resource::configuration::Request> : detail::type< transaction::resource::configuration::Reply> {};

      template<>
      struct type_traits< transaction::inbound::branch::Request> : detail::type< transaction::inbound::branch::Reply> {};

      template<>
      struct type_traits< transaction::active::Request> : detail::type< transaction::active::Reply> {};

   } // common::message::reverse
} // casual
