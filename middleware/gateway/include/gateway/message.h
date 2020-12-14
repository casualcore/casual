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

#include "configuration/message.h"

#include <thread>




namespace casual
{
   namespace gateway::message
   {
      using size_type = platform::size::type;

      struct Address
      {
         std::string local;
         std::string peer;

         CASUAL_CONST_CORRECT_SERIALIZE(
         {
            CASUAL_SERIALIZE( local);
            CASUAL_SERIALIZE( peer);
         })
      };

      namespace state
      {
         struct Connection
         {
            Address address;

         };
      } // state


      template< common::message::Type type>
      struct basic_connect : common::message::basic_request< type>
      {
         using base_connect = common::message::basic_request< type>;
         using base_connect::base_connect;

         common::domain::Identity domain;
         common::message::gateway::domain::protocol::Version version;
         Address address;

         CASUAL_CONST_CORRECT_SERIALIZE({
            base_connect::serialize( archive);
            CASUAL_SERIALIZE( domain);
            CASUAL_SERIALIZE( version);
            CASUAL_SERIALIZE( address);
         })
      };

      namespace outbound
      {
         template< typename Base>
         struct basic_configuration : Base
         {
            using Base::Base;

            std::vector< std::string> services;
            std::vector< std::string> queues;

            CASUAL_CONST_CORRECT_SERIALIZE(
               Base::serialize( archive);
               CASUAL_SERIALIZE( services);
               CASUAL_SERIALIZE( queues);
            )
         };

         namespace configuration
         {
            using Request = common::message::basic_request< common::message::Type::gateway_outbound_configuration_request>;
            using Reply = basic_configuration< common::message::basic_reply< common::message::Type::gateway_outbound_configuration_reply>>;

         } // configuration

         using Connect  = basic_connect< common::message::Type::gateway_outbound_connect>;

         namespace connect
         {
            using Done = common::message::basic_message< common::message::Type::gateway_outbound_connect_done>;

         } // connect

         namespace rediscover
         {
            using Request = basic_configuration< common::message::basic_request< common::message::Type::gateway_outbound_rediscover_request>>;
            using Reply = common::message::basic_reply< common::message::Type::gateway_outbound_rediscover_reply>;
         } // rediscover

      } // outbound

      namespace inbound
      {
         using Connect = basic_connect< common::message::Type::gateway_inbound_connect>;
         
      } // inbound


      namespace reverse
      {
         namespace inbound
         {
            struct State
            {
               CASUAL_CONST_CORRECT_SERIALIZE(
              
               )
            };

            using Connect = common::message::basic_request< common::message::Type::gateway_reverse_inbound_connect>;

            namespace configuration::update
            {
               using base_request = common::message::basic_request< common::message::Type::gateway_reverse_inbound_configuration_update_request>;
               struct Request : base_request
               {
                  using base_request::base_request;

                  casual::configuration::model::gateway::reverse::Inbound model;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_request::serialize( archive);
                     CASUAL_SERIALIZE( model);
                  )
               };

               using Reply = common::message::basic_message< common::message::Type::gateway_reverse_inbound_configuration_update_reply>;
               
            } // configuration::update

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


         } // inbound

         namespace outbound
         {
            namespace state
            {
               struct Listener
               {
                  common::communication::tcp::Address address;
                  common::strong::file::descriptor::id descriptor;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     CASUAL_SERIALIZE( address);
                     CASUAL_SERIALIZE( descriptor);
                  })
               };

               struct Address
               {
                  common::communication::tcp::Address local;
                  common::communication::tcp::Address peer;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     CASUAL_SERIALIZE( local);
                     CASUAL_SERIALIZE( peer);
                  })
               };

               struct Connection
               {
                  Address address;
                  common::strong::file::descriptor::id descriptor;
                  common::domain::Identity domain;
                  configuration::model::gateway::reverse::outbound::Connection configuration;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     CASUAL_SERIALIZE( address);
                     CASUAL_SERIALIZE( descriptor);
                     CASUAL_SERIALIZE( domain);
                     CASUAL_SERIALIZE( configuration);
                  })
               };
               
            } // state

            struct State
            {
               std::string alias;
               platform::size::type order{};

               std::vector< state::Connection> connections;
               std::vector< state::Listener> listeners;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  CASUAL_SERIALIZE( alias);
                  CASUAL_SERIALIZE( order);
                  CASUAL_SERIALIZE( connections);
                  CASUAL_SERIALIZE( listeners);
               })
            };


            using Connect = common::message::basic_request< common::message::Type::gateway_reverse_outbound_connect>;

            namespace configuration::update
            {
               using base_request = common::message::basic_request< common::message::Type::gateway_reverse_outbound_configuration_update_request>;
               struct Request : base_request
               {
                  using base_request::base_request;

                  casual::configuration::model::gateway::reverse::Outbound model;
                  platform::size::type order{};

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_request::serialize( archive);
                     CASUAL_SERIALIZE( model);
                     CASUAL_SERIALIZE( order);
                  )
               };

               using Reply = common::message::basic_request< common::message::Type::gateway_reverse_outbound_configuration_update_reply>;
               
            } // configuration::update

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
            
         } // outbound

      } // reverse

   } // gateway::message

   namespace common
   {
      namespace message::reverse
      {
         template<>
         struct type_traits< casual::gateway::message::outbound::configuration::Request> : detail::type< casual::gateway::message::outbound::configuration::Reply> {};

         template<>
         struct type_traits< casual::gateway::message::outbound::rediscover::Request> : detail::type< casual::gateway::message::outbound::rediscover::Reply> {};

         template<>
         struct type_traits< casual::gateway::message::reverse::inbound::configuration::update::Request> : detail::type< casual::gateway::message::reverse::inbound::configuration::update::Reply> {};

         template<>
         struct type_traits< casual::gateway::message::reverse::outbound::configuration::update::Request> : detail::type< casual::gateway::message::reverse::outbound::configuration::update::Reply> {};

         template<>
         struct type_traits< casual::gateway::message::reverse::outbound::state::Request> : detail::type< casual::gateway::message::reverse::outbound::state::Reply> {};

         template<>
         struct type_traits< casual::gateway::message::reverse::inbound::state::Request> : detail::type< casual::gateway::message::reverse::inbound::state::Reply> {};
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

         CASUAL_CUSTOMIZATION_POINT_NETWORK( common::message::gateway::domain::connect::Request,
         {
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( execution);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( domain);
            CASUAL_SERIALIZE_NAME( value.versions, "protocol.versions");
         })

         CASUAL_CUSTOMIZATION_POINT_NETWORK( common::message::gateway::domain::connect::Reply,
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




