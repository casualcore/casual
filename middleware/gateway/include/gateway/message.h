//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MESSAGE_H_
#define CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MESSAGE_H_


#include "gateway/message.h"

#include "common/message/type.h"
#include "common/message/transaction.h"
#include "common/message/service.h"
#include "common/message/gateway.h"
#include "common/message/queue.h"
#include "common/message/conversation.h"
#include "common/domain.h"
#include "common/marshal/binary.h"

#include <thread>





//!
//! global overload for XID
//!
//! @{
template< typename M>
void casual_marshal_value( const XID& value, M& marshler)
{
   marshler << value.formatID;

   if( ! casual::common::transaction::null( value))
   {
      marshler << value.gtrid_length;
      marshler << value.bqual_length;

      marshler.append( casual::common::transaction::data( value));
   }
}

template< typename M>
void casual_unmarshal_value( XID& value, M& unmarshler)
{
   unmarshler >> value.formatID;

   if( ! casual::common::transaction::null( value))
   {
      unmarshler >> value.gtrid_length;
      unmarshler >> value.bqual_length;

      unmarshler.consume(
         std::begin( value.data),
         value.gtrid_length + value.bqual_length);
   }
}
//! @}


#define CASUAL_CUSTOMIZATION_POINT_MARSHAL( type, statement) \
template< typename A> auto casual_marshal( type& value, A& archive) -> std::enable_if_t< casual::common::marshal::is_network_normalizing< A>::value> \
{  \
   statement  \
} \
template< typename A> auto casual_marshal( const type& value, A& archive) -> std::enable_if_t< casual::common::marshal::is_network_normalizing< A>::value> \
{  \
   statement  \
} \

namespace casual
{
   namespace gateway
   {
      namespace message
      {
         using size_type = common::platform::size::type;

         namespace manager
         {

            namespace listener
            {
               struct Event : common::message::basic_message< common::message::Type::gateway_manager_listener_event>
               {

                  enum class State
                  {
                     running,
                     exit,
                     signal,
                     error
                  };

                  State state;

                  CASUAL_CONST_CORRECT_MARSHAL({
                     base_type::marshal( archive);
                     archive & state;
                  })

                  friend std::ostream& operator << ( std::ostream& out, const Event::State& value);
                  friend std::ostream& operator << ( std::ostream& out, const Event& value);
               };

            } // listener
         } // manager




         template< common::message::Type type>
         struct basic_connect : common::message::basic_message< type>
         {
            common::process::Handle process;
            common::domain::Identity domain;
            common::message::gateway::domain::protocol::Version version;
            std::vector< std::string> address;

            CASUAL_CONST_CORRECT_MARSHAL({
               common::message::basic_message< type>::marshal( archive);
               archive & process;
               archive & domain;
               archive & version;
               archive & address;
            })

            friend std::ostream& operator << ( std::ostream& out, const basic_connect& value)
            {
               return out << "{ process: " << value.process
                     << ", address: " << common::range::make( value.address)
                     << ", domain: " << value.domain
                     << '}';
            }
         };

         namespace outbound
         {
            namespace configuration
            {

               struct Request : common::message::server::basic_id< common::message::Type::gateway_outbound_configuration_request>
               {

                  friend std::ostream& operator << ( std::ostream& out, const Request& value);
               };


               using base_reply = common::message::server::basic_id< common::message::Type::gateway_outbound_configuration_reply>;
               struct Reply : base_reply
               {
                  std::vector< std::string> services;
                  std::vector< std::string> queues;

                  CASUAL_CONST_CORRECT_MARSHAL({
                     base_reply::marshal( archive);
                     archive & services;
                     archive & queues;
                  })

                  friend std::ostream& operator << ( std::ostream& out, const Reply& value);
               };

            } // configuration

            struct Connect : basic_connect< common::message::Type::gateway_outbound_connect>
            {
            };

         } // outbound

         namespace inbound
         {
            using base_connect =  basic_connect< common::message::Type::gateway_inbound_connect>;
            struct Connect : base_connect
            {
            };
         } // inbound

         namespace ipc
         {
            namespace connect
            {

               struct Request : basic_connect< common::message::Type::gateway_ipc_connect_request>
               {
                  friend std::ostream& operator << ( std::ostream& out, const Request& value);
               };

               struct Reply : basic_connect< common::message::Type::gateway_ipc_connect_reply>
               {
                  friend std::ostream& operator << ( std::ostream& out, const Reply& value);
               };

            } // connect

         } // ipc

         namespace tcp
         {
            namespace connect
            {
               struct Limit
               {
                  size_type size = 0;
                  size_type messages = 0;

                  CASUAL_CONST_CORRECT_MARSHAL(
                     archive & size;
                     archive & messages;
                  )

               };
            } // connect
            struct Connect : common::message::basic_message< common::message::Type::gateway_manager_tcp_connect>
            {
               common::platform::tcp::descriptor::type descriptor;
               connect::Limit limit;

               friend std::ostream& operator << ( std::ostream& out, const Connect& value);

               CASUAL_CONST_CORRECT_MARSHAL({
                  base_type::marshal( archive);
                  archive & descriptor;
                  archive & limit;
               })
            };

         } // tcp


         namespace worker
         {

            struct Connect : common::message::basic_message< common::message::Type::gateway_worker_connect>
            {
               common::platform::binary::type information;

               CASUAL_CONST_CORRECT_MARSHAL({
                  base_type::marshal( archive);
                  archive & information;
               })

            };

            struct Disconnect : common::message::basic_message< common::message::Type::gateway_worker_disconnect>
            {
               enum class Reason : char
               {
                  invalid,
                  signal,
                  disconnect
               };

               Disconnect() = default;
               Disconnect( Reason reason) : reason( reason) {}

               common::domain::Identity remote;
               Reason reason = Reason::invalid;

               friend std::ostream& operator << ( std::ostream& out, Reason value);
               friend std::ostream& operator << ( std::ostream& out, const Disconnect& value);

               CASUAL_CONST_CORRECT_MARSHAL({
                  base_type::marshal( archive);
                  archive & remote;
                  archive & reason;
               })

            };


         } // worker
      } // message
   } // gateway

   namespace common
   {
      namespace message
      {
         namespace reverse
         {
            template<>
            struct type_traits< casual::gateway::message::ipc::connect::Request> : detail::type< casual::gateway::message::ipc::connect::Reply> {};

            template<>
            struct type_traits< casual::gateway::message::outbound::configuration::Request> : detail::type< casual::gateway::message::outbound::configuration::Reply> {};
         } // reverse

         namespace gateway
         {
            namespace domain
            { 
               namespace connect
               {
                  CASUAL_CUSTOMIZATION_POINT_MARSHAL( Request,
                  {
                     archive & value.execution;
                     archive & value.domain;
                     archive & value.versions;
                  })

                  CASUAL_CUSTOMIZATION_POINT_MARSHAL( Reply,
                  {
                     archive & value.execution;
                     archive & value.domain;
                     archive & value.version;
                  })
               } // connect

               namespace discover
               {
                  CASUAL_CUSTOMIZATION_POINT_MARSHAL( Request,
                  {
                     archive & value.execution;
                     archive & value.domain;
                     archive & value.services;
                     archive & value.queues;
                  })

                  CASUAL_CUSTOMIZATION_POINT_MARSHAL( Reply,
                  {
                     archive & value.execution;
                     archive & value.domain;
                     archive & value.services;
                     archive & value.queues;
                  })
               }
            }
         }

         namespace service 
         {
            namespace call 
            { 
               CASUAL_CUSTOMIZATION_POINT_MARSHAL( callee::Request,
               {
                  archive & value.execution;
                  archive & value.service.name;
                  archive & value.service.timeout;
                  archive & value.parent;
                  archive & value.trid.xid;
                  archive & value.flags;
                  archive & value.buffer;
               })
            
               CASUAL_CUSTOMIZATION_POINT_MARSHAL( Reply,
               {
                     archive & value.execution;
                     archive & value.status;
                     archive & value.code;
                     archive & value.transaction.trid.xid;
                     archive & value.transaction.state;
                     archive & value.buffer;
               })
            }
         }

         namespace conversation
         {
            namespace connect
            {
               CASUAL_CUSTOMIZATION_POINT_MARSHAL( callee::Request,
               {
                  archive & value.execution;
                  archive & value.service.name;
                  archive & value.service.timeout;
                  archive & value.parent;
                  archive & value.trid.xid;
                  archive & value.flags;
                  archive & value.recording;
                  archive & value.buffer;
               })

               CASUAL_CUSTOMIZATION_POINT_MARSHAL( Reply,
               {
                  archive & value.execution;
                  archive & value.route;
                  archive & value.recording;
                  archive & value.status;
               })

            } // connect

            CASUAL_CUSTOMIZATION_POINT_MARSHAL( Disconnect,
            {
               archive & value.execution;
               archive & value.route;
               archive & value.events;
            })

            CASUAL_CUSTOMIZATION_POINT_MARSHAL( callee::Send,
            {
               archive & value.execution;
               archive & value.route;
               archive & value.events;
               archive & value.status;
               archive & value.buffer;
            })

         } // conversation

         namespace transaction
         {
            namespace resource
            {
               namespace marshal
               {
                  template< typename T, typename A>
                  void transaction_request( T& value, A& archive)
                  {
                     archive & value.execution;
                     archive & value.trid.xid;
                     archive & value.resource;
                     archive & value.flags;
                  }

                  template< typename T, typename A>
                  void transaction_reply( T& value, A& archive)
                  {
                     archive & value.execution;
                     archive & value.trid.xid;
                     archive & value.resource;
                     archive & value.state;
                  }

               } // marshal

               CASUAL_CUSTOMIZATION_POINT_MARSHAL( prepare::Request,
               {
                  marshal::transaction_request( value, archive);
               })

               CASUAL_CUSTOMIZATION_POINT_MARSHAL( prepare::Reply,
               {
                  marshal::transaction_reply( value, archive);
               })

               CASUAL_CUSTOMIZATION_POINT_MARSHAL( commit::Request,
               {
                  marshal::transaction_request( value, archive);
               })

               CASUAL_CUSTOMIZATION_POINT_MARSHAL( commit::Reply,
               {
                  marshal::transaction_reply( value, archive);
               })

               CASUAL_CUSTOMIZATION_POINT_MARSHAL( rollback::Request, 
               {
                  marshal::transaction_request( value, archive);
               })

               CASUAL_CUSTOMIZATION_POINT_MARSHAL( rollback::Reply,
               {
                  marshal::transaction_reply( value, archive);
               })

            } // resource
         } // transaction

         namespace queue
         {
            namespace enqueue
            {
               CASUAL_CUSTOMIZATION_POINT_MARSHAL( Request,
               {
                  archive & value.execution;
                  archive & value.name;
                  archive & value.trid.xid;
                  archive & value.message;
               })

               CASUAL_CUSTOMIZATION_POINT_MARSHAL( Reply,
               {
                  archive & value.execution;
                  archive & value.id;
               })
            }

            namespace dequeue
            {

               CASUAL_CUSTOMIZATION_POINT_MARSHAL( Request,
               {
                  archive & value.execution;
                  archive & value.name;
                  archive & value.trid.xid;
                  archive & value.selector;
                  archive & value.block;
               })


               CASUAL_CUSTOMIZATION_POINT_MARSHAL( Reply,
               {
                  archive & value.execution;
                  archive & value.message;
               })               
            }
         } // queue
      } // message
   } // common


} // casual



#endif // CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MESSAGE_H_
