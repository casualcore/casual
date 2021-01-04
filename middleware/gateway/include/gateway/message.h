//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "gateway/manager/admin/model.h"

#include "common/message/type.h"
#include "common/message/transaction.h"
#include "common/message/service.h"
#include "common/message/gateway.h"
#include "common/message/queue.h"
#include "common/message/conversation.h"
#include "common/domain.h"
#include "common/serialize/native/binary.h"
#include "common/serialize/value/customize.h"
#include "common/communication/tcp.h"

#include "configuration/model.h"


namespace casual
{
   namespace gateway::message
   {
      using size_type = platform::size::type;

      namespace domain
      {
         namespace protocol
         {
            enum class Version : size_type
            {
               invalid = 0,
               version_1 = 1000,
               version_1_1 = 1001,

               // make sure this 'points' to the latest version
               latest = version_1_1
            };

            //! an array with all versions ordered by highest to lowest
            constexpr auto versions = std::array< Version, 2>{ Version::version_1_1, Version::version_1};
         } // protocol

         namespace connect
         {
            using base_request = common::message::basic_message< common::message::Type::gateway_domain_connect_request>;
            struct Request : base_request
            {
               using base_request::base_request;

               common::domain::Identity domain;
               std::vector< protocol::Version> versions;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  base_request::serialize( archive);
                  CASUAL_SERIALIZE( domain);
                  CASUAL_SERIALIZE( versions);
               })
            };

            using base_reply = common::message::basic_message< common::message::Type::gateway_domain_connect_reply>;
            struct Reply : base_reply
            {
               using base_reply::base_reply;

               common::domain::Identity domain;
               protocol::Version version = protocol::Version::invalid;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  base_reply::serialize( archive);
                  CASUAL_SERIALIZE( domain);
                  CASUAL_SERIALIZE( version);
               })
            };
         } // connect

      } // domain

      namespace state
      {
         struct Address
         {
            common::communication::tcp::Address local;
            common::communication::tcp::Address peer;
            common::strong::file::descriptor::id descriptor;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( local);
               CASUAL_SERIALIZE( peer);
               CASUAL_SERIALIZE( descriptor);
            )
         };

         struct Listener
         {
            common::communication::tcp::Address address;
            common::strong::file::descriptor::id descriptor;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( address);
               CASUAL_SERIALIZE( descriptor);
            )
         };

         template< typename Configuration>
         struct basic_connection
         {
            Address address;
            common::strong::file::descriptor::id descriptor;
            common::domain::Identity domain;
            Configuration configuration;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( address);
               CASUAL_SERIALIZE( descriptor);
               CASUAL_SERIALIZE( domain);
               CASUAL_SERIALIZE( configuration);
            )
         };
         
      } // state

      namespace inbound
      {
         namespace state
         {
            using Connection = message::state::basic_connection< casual::configuration::model::gateway::inbound::Connection>;
            
         } // state

         struct base_state
         {
            std::string alias;
            std::string note;
            casual::configuration::model::gateway::inbound::Limit limit;
            std::vector< state::Connection> connections;

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               CASUAL_SERIALIZE( alias);
               CASUAL_SERIALIZE( note);
               CASUAL_SERIALIZE( limit);
               CASUAL_SERIALIZE( connections);
            })
         };

         struct State : base_state
         {
            using base_state::base_state;

            std::vector< message::state::Listener> listeners;

            CASUAL_CONST_CORRECT_SERIALIZE(
               base_state::serialize( archive);
               CASUAL_SERIALIZE( listeners);
            )

         };

         using Connect = common::message::basic_request< common::message::Type::gateway_inbound_connect>;

         namespace configuration::update
         {
            using base_request = common::message::basic_request< common::message::Type::gateway_inbound_configuration_update_request>;
            struct Request : base_request
            {
               using base_request::base_request;

               casual::configuration::model::gateway::Inbound model;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_request::serialize( archive);
                  CASUAL_SERIALIZE( model);
               )
            };

            using Reply = common::message::basic_request< common::message::Type::gateway_inbound_configuration_update_reply>;
            
         } // configuration::update

         namespace state
         {            
            using Request = common::message::basic_request< common::message::Type::gateway_inbound_state_request>;

            using base_reply = common::message::basic_request< common::message::Type::gateway_inbound_state_reply>;
            struct Reply : base_reply
            {
               using base_reply::base_reply;

               State state;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_reply::serialize( archive);
                  CASUAL_SERIALIZE( state);
               )
            };
            
         } // state

         //! different state for reverse
         namespace reverse
         {
            struct State : inbound::base_state
            {
               using inbound::base_state::base_state;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  inbound::base_state::serialize( archive);
               )
            };

            namespace state
            {
               using Request = common::message::basic_request< common::message::Type::gateway_reverse_inbound_state_request>;

               using base_reply = common::message::basic_request< common::message::Type::gateway_reverse_inbound_state_reply>;
               struct Reply : base_reply
               {
                  using base_reply::base_reply;

                  State state;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_reply::serialize( archive);
                     CASUAL_SERIALIZE( state);
                  )
               };
            } // state
         } // reverse

      } // inbound

      namespace outbound
      {
         namespace state
         {
            using Connection = message::state::basic_connection< casual::configuration::model::gateway::outbound::Connection>;
            
         } // state

         struct State
         {
            std::string alias;
            std::string note;
            platform::size::type order{};

            std::vector< state::Connection> connections;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( alias);
               CASUAL_SERIALIZE( note);
               CASUAL_SERIALIZE( order);
               CASUAL_SERIALIZE( connections);
            )
         };


         using Connect = common::message::basic_request< common::message::Type::gateway_outbound_connect>;

         namespace configuration::update
         {
            using base_request = common::message::basic_request< common::message::Type::gateway_outbound_configuration_update_request>;
            struct Request : base_request
            {
               using base_request::base_request;

               casual::configuration::model::gateway::Outbound model;
               platform::size::type order{};

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_request::serialize( archive);
                  CASUAL_SERIALIZE( model);
                  CASUAL_SERIALIZE( order);
               )
            };

            using Reply = common::message::basic_request< common::message::Type::gateway_outbound_configuration_update_reply>;
            
         } // configuration::update

         namespace state
         {
            using Request = common::message::basic_request< common::message::Type::gateway_outbound_state_request>;

            using base_reply = common::message::basic_request< common::message::Type::gateway_outbound_state_reply>;
            struct Reply : base_reply
            {
               using base_reply::base_reply;

               State state;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_reply::serialize( archive);
                  CASUAL_SERIALIZE( state);
               )
            };
            
         } // state

         namespace rediscover
         {
            using Request = common::message::basic_request< common::message::Type::gateway_outbound_rediscover_request>;
            using Reply = common::message::basic_reply< common::message::Type::gateway_outbound_rediscover_reply>;
         } // rediscover


         //! different state for reverse
         namespace reverse
         {
            struct State : message::outbound::State
            {
               using message::outbound::State::State;

               std::vector< message::state::Listener> listeners;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  message::outbound::State::serialize( archive);
                  CASUAL_SERIALIZE( listeners);
               })
            };

            namespace state
            {
               using Request = common::message::basic_request< common::message::Type::gateway_reverse_outbound_state_request>;

               using base_reply = common::message::basic_request< common::message::Type::gateway_reverse_outbound_state_reply>;
               struct Reply : base_reply
               {
                  using base_reply::base_reply;

                  State state;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_reply::serialize( archive);
                     CASUAL_SERIALIZE( state);
                  )
               };
               
            } // state
         } // resverse
         
      } // outbound


   } // gateway::message

   namespace common
   {
      namespace message::reverse
      {

         template<>
         struct type_traits< casual::gateway::message::domain::connect::Request> : detail::type<  casual::gateway::message::domain::connect::Reply> {};

         template<>
         struct type_traits< casual::gateway::message::outbound::rediscover::Request> : detail::type< casual::gateway::message::outbound::rediscover::Reply> {};

         template<>
         struct type_traits< casual::gateway::message::inbound::configuration::update::Request> : detail::type< casual::gateway::message::inbound::configuration::update::Reply> {};

         template<>
         struct type_traits< casual::gateway::message::outbound::configuration::update::Request> : detail::type< casual::gateway::message::outbound::configuration::update::Reply> {};

         template<>
         struct type_traits< casual::gateway::message::outbound::state::Request> : detail::type< casual::gateway::message::outbound::state::Reply> {};

         template<>
         struct type_traits< casual::gateway::message::inbound::state::Request> : detail::type< casual::gateway::message::inbound::state::Reply> {};

         template<>
         struct type_traits< casual::gateway::message::outbound::reverse::state::Request> : detail::type< casual::gateway::message::outbound::reverse::state::Reply> {};

         template<>
         struct type_traits< casual::gateway::message::inbound::reverse::state::Request> : detail::type< casual::gateway::message::inbound::reverse::state::Reply> {};
      } // message::reverse

      namespace serialize::customize::composit
      {

//! customization macro to make it easier to define customization points for all interdomain messages
#define CASUAL_CUSTOMIZATION_POINT_NETWORK( type, statement) \
template< typename A> struct Value< type, A, std::enable_if_t< common::serialize::traits::is::network::normalizing< A>::value>>  \
{ \
   template< typename V> static void serialize( A& archive, V&& value) \
   {  \
      statement  \
   } \
}; 
         

#define CASUAL_CUSTOMIZATION_POINT_SERIALIZE( role) CASUAL_SERIALIZE_NAME( value.role, #role)

         CASUAL_CUSTOMIZATION_POINT_NETWORK( casual::gateway::message::domain::connect::Request,
         {
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( execution);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( domain);
            CASUAL_SERIALIZE_NAME( value.versions, "protocol.versions");
         })

         CASUAL_CUSTOMIZATION_POINT_NETWORK( casual::gateway::message::domain::connect::Reply,
         {
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( execution);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( domain);
            CASUAL_SERIALIZE_NAME( value.version, "protocol.version");
         })

         CASUAL_CUSTOMIZATION_POINT_NETWORK( common::message::gateway::domain::discover::Request,
         {
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( execution);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( domain);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( services);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( queues);
         })
         
         CASUAL_CUSTOMIZATION_POINT_NETWORK( common::message::gateway::domain::discover::Reply::Service,
         {
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( name);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( category);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( transaction);
            // TODO 2.0
            // CASUAL_CUSTOMIZATION_POINT_SERIALIZE( type);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( timeout);
            CASUAL_SERIALIZE_NAME( value.property.hops, "hops");
         })

         CASUAL_CUSTOMIZATION_POINT_NETWORK( common::message::gateway::domain::discover::Reply,
         {
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( execution);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( domain);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( services);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( queues);
         })

         CASUAL_CUSTOMIZATION_POINT_NETWORK( common::message::service::call::callee::Request,
         {
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( execution);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( service.name);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( service.timeout);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( parent);
            CASUAL_SERIALIZE_NAME( value.trid.xid, "xid");
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( flags);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( buffer);
         })
      
         CASUAL_CUSTOMIZATION_POINT_NETWORK( common::message::service::call::Reply,
         {
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( execution);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( code.result);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( code.user);
            CASUAL_SERIALIZE_NAME( value.transaction.trid.xid, "transaction.xid");
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( transaction.state);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( buffer);
         })

         CASUAL_CUSTOMIZATION_POINT_NETWORK( common::message::conversation::connect::callee::Request,
         {
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( execution);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( service.name);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( service.timeout);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( parent);
            CASUAL_SERIALIZE_NAME( value.trid.xid, "xid");
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( flags);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( recording);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( buffer);
         })

         CASUAL_CUSTOMIZATION_POINT_NETWORK( common::message::conversation::connect::Reply,
         {
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( execution);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( route);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( recording);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( code.result);
            // TODO: CASUAL_CUSTOMIZATION_POINT_SERIALIZE( code.user);   
         })


         CASUAL_CUSTOMIZATION_POINT_NETWORK( common::message::conversation::Disconnect,
         {
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( execution);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( route);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( events);
         })

         CASUAL_CUSTOMIZATION_POINT_NETWORK( common::message::conversation::callee::Send,
         {
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( execution);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( route);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( events);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( code.result);
            // TODO: CASUAL_CUSTOMIZATION_POINT_SERIALIZE( code.user);  
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( buffer);
         })


         namespace detail
         {
            template< typename T, typename A>
            void transaction_request( T& value, A& archive)
            {
               CASUAL_CUSTOMIZATION_POINT_SERIALIZE( execution);
               CASUAL_SERIALIZE_NAME( value.trid.xid, "xid");
               CASUAL_CUSTOMIZATION_POINT_SERIALIZE( resource);
               CASUAL_CUSTOMIZATION_POINT_SERIALIZE( flags);
            }

            template< typename T, typename A>
            void transaction_reply( T& value, A& archive)
            {
               CASUAL_CUSTOMIZATION_POINT_SERIALIZE( execution);
               CASUAL_SERIALIZE_NAME( value.trid.xid, "xid");
               CASUAL_CUSTOMIZATION_POINT_SERIALIZE( resource);
               CASUAL_CUSTOMIZATION_POINT_SERIALIZE( state);
            }

         } // detail

         CASUAL_CUSTOMIZATION_POINT_NETWORK( common::message::transaction::resource::prepare::Request,
         {
            detail::transaction_request( value, archive);
         })

         CASUAL_CUSTOMIZATION_POINT_NETWORK( common::message::transaction::resource::prepare::Reply,
         {
            detail::transaction_reply( value, archive);
         })

         CASUAL_CUSTOMIZATION_POINT_NETWORK( common::message::transaction::resource::commit::Request,
         {
            detail::transaction_request( value, archive);
         })

         CASUAL_CUSTOMIZATION_POINT_NETWORK( common::message::transaction::resource::commit::Reply,
         {
            detail::transaction_reply( value, archive);
         })

         CASUAL_CUSTOMIZATION_POINT_NETWORK( common::message::transaction::resource::rollback::Request, 
         {
            detail::transaction_request( value, archive);
         })

         CASUAL_CUSTOMIZATION_POINT_NETWORK( common::message::transaction::resource::rollback::Reply,
         {
            detail::transaction_reply( value, archive);
         })

         CASUAL_CUSTOMIZATION_POINT_NETWORK( common::message::queue::enqueue::Request,
         {
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( execution);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( name);
            CASUAL_SERIALIZE_NAME( value.trid.xid, "xid");
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( message);
         })

         CASUAL_CUSTOMIZATION_POINT_NETWORK( common::message::queue::enqueue::Reply,
         {
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( execution);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( id);
         })
      

         CASUAL_CUSTOMIZATION_POINT_NETWORK( common::message::queue::dequeue::Request,
         {
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( execution);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( name);
            CASUAL_SERIALIZE_NAME( value.trid.xid, "xid");
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( selector);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( block);
         })


         CASUAL_CUSTOMIZATION_POINT_NETWORK( common::message::queue::dequeue::Reply,
         {
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( execution);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( message);
         })
         
      } // serialize::customize::composit
   } // common
} // casual




