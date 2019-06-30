//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/message/type.h"
#include "common/message/transaction.h"
#include "common/message/service.h"
#include "common/message/gateway.h"
#include "common/message/queue.h"
#include "common/message/conversation.h"
#include "common/domain.h"
#include "common/serialize/native/binary.h"
#include "common/serialize/value/customize.h"

#include <thread>




namespace casual
{
   namespace gateway
   {
      namespace message
      {
         using size_type = common::platform::size::type;

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


         template< common::message::Type type>
         struct basic_connect : common::message::basic_message< type>
         {
            common::process::Handle process;
            common::domain::Identity domain;
            common::message::gateway::domain::protocol::Version version;
            Address address;

            CASUAL_CONST_CORRECT_SERIALIZE({
               common::message::basic_message< type>::serialize( archive);
               CASUAL_SERIALIZE( process);
               CASUAL_SERIALIZE( domain);
               CASUAL_SERIALIZE( version);
               CASUAL_SERIALIZE( address);
            })
         };

         namespace outbound
         {
            namespace configuration
            {

               struct Request : common::message::server::basic_id< common::message::Type::gateway_outbound_configuration_request>
               {};


               using base_reply = common::message::server::basic_id< common::message::Type::gateway_outbound_configuration_reply>;
               struct Reply : base_reply
               {
                  std::vector< std::string> services;
                  std::vector< std::string> queues;

                  CASUAL_CONST_CORRECT_SERIALIZE({
                     base_reply::serialize( archive);
                     CASUAL_SERIALIZE( services);
                     CASUAL_SERIALIZE( queues);
                  })
               };

            } // configuration

            struct Connect : basic_connect< common::message::Type::gateway_outbound_connect>
            {
            };

            namespace connect
            {
               struct Done : common::message::basic_message< common::message::Type::gateway_outbound_connect_done>
               {
                  common::strong::socket::id descriptor;

                  CASUAL_CONST_CORRECT_SERIALIZE({
                     base_type::serialize( archive);
                     CASUAL_SERIALIZE( descriptor);
                  })
               };
            } // connect

         } // outbound

         namespace inbound
         {
            struct Limit
            {
               size_type size = 0;
               size_type messages = 0;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( size);
                  CASUAL_SERIALIZE( messages);
               )
            };

            using base_connect =  basic_connect< common::message::Type::gateway_inbound_connect>;
            struct Connect : base_connect
            {
            };
         } // inbound

      } // message
   } // gateway

   namespace common
   {

      namespace message
      {
         namespace reverse
         {
            template<>
            struct type_traits< casual::gateway::message::outbound::configuration::Request> : detail::type< casual::gateway::message::outbound::configuration::Reply> {};
         } // reverse
      } // message

      namespace serialize
      {
         namespace customize
         {
            namespace composit
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

            } // composit
            
         } // customize
         
      } // serialize
   } // common
} // casual




