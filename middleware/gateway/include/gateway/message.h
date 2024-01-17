//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "gateway/message/protocol.h"
#include "gateway/manager/admin/model.h"

#include "domain/message/discovery.h"
#include "queue/common/ipc/message.h"

#include "common/message/type.h"
#include "common/message/transaction.h"
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
      namespace domain
      {
         namespace connect
         {
            using base_request = common::message::basic_message< common::message::Type::gateway_domain_connect_request>;
            struct Request : base_request
            {
               using base_request::base_request;

               common::domain::Identity domain;
               std::vector< protocol::Version> versions;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_request::serialize( archive);
                  CASUAL_SERIALIZE( domain);
                  CASUAL_SERIALIZE( versions);
               )
            };

            using base_reply = common::message::basic_message< common::message::Type::gateway_domain_connect_reply>;
            struct Reply : base_reply
            {
               using base_reply::base_reply;

               common::domain::Identity domain;
               protocol::Version version = protocol::Version::invalid;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_reply::serialize( archive);
                  CASUAL_SERIALIZE( domain);
                  CASUAL_SERIALIZE( version);
               )
            };
         } // connect

         namespace disconnect
         {
            using Request = common::message::basic_message< common::message::Type::gateway_domain_disconnect_request>;
            using Reply = common::message::basic_message< common::message::Type::gateway_domain_disconnect_reply>;

         } // disconnect

         //! Sent from group-connector when the logical connection is done
         using base_connected = common::message::basic_message< common::message::Type::gateway_domain_connected>;
         struct Connected : base_connected
         {
            using base_connected::base_connected;

            common::strong::process::id connector;
            common::domain::Identity domain;
            protocol::Version version = protocol::Version::invalid;

            CASUAL_CONST_CORRECT_SERIALIZE(
               base_connected::serialize( archive);
               CASUAL_SERIALIZE( connector);
               CASUAL_SERIALIZE( domain);
               CASUAL_SERIALIZE( version);
            )
         };
            
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

         namespace listener
         {
            enum struct Runlevel : std::uint8_t
            {
               listening,
               failed,
            };
            constexpr std::string_view description( Runlevel value) noexcept
            {
               switch( value)
               {
                  case Runlevel::listening: return "listening";
                  case Runlevel::failed: return "failed";
               }
               return "<unknown>";
            }
         } // listener

         struct Listener
         {
            listener::Runlevel runlevel{};
            common::communication::tcp::Address address;
            common::strong::file::descriptor::id descriptor;
            platform::time::point::type created{};

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( runlevel);
               CASUAL_SERIALIZE( address);
               CASUAL_SERIALIZE( descriptor);
               CASUAL_SERIALIZE( created);
            )
         };

         namespace connection
         {
            enum struct Runlevel : std::uint8_t
            {
               connecting,
               pending,
               connected,
               failed,
            };
            constexpr std::string_view description( Runlevel value) noexcept
            {
               switch( value)
               {
                  case Runlevel::connecting: return "connecting";
                  case Runlevel::pending: return "pending";
                  case Runlevel::connected: return "connected";
                  case Runlevel::failed: return "failed";
               }
               return "<unknown>";
            }
         } // connection

         template< typename Configuration>
         struct basic_connection
         {
            Address address;
            connection::Runlevel runlevel{};
            common::strong::file::descriptor::id descriptor;
            common::domain::Identity domain;
            Configuration configuration;
            platform::time::point::type created{};

            struct
            {
               struct Metric
               {
                  platform::size::type count{};
                  platform::size::type pending{};

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( count);
                     CASUAL_SERIALIZE( pending);
                  )
               };
               
               Metric send;
               Metric receive;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( send);
                  CASUAL_SERIALIZE( receive);
               )
               
            } metric;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( address);
               CASUAL_SERIALIZE( runlevel);
               CASUAL_SERIALIZE( descriptor);
               CASUAL_SERIALIZE( domain);
               CASUAL_SERIALIZE( configuration);
               CASUAL_SERIALIZE( metric);
               CASUAL_SERIALIZE( created);
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

               casual::configuration::model::gateway::inbound::Group model;

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

         namespace connection
         {
            using base_lost = common::message::basic_message< common::message::Type::gateway_inbound_connection_lost>;
            struct Lost : base_lost
            {
               Lost() = default;
               Lost( casual::configuration::model::gateway::inbound::Connection configuration, common::domain::Identity remote)
                  : configuration{ std::move( configuration)}, remote{ std::move( remote)} {}

               casual::configuration::model::gateway::inbound::Connection configuration;
               common::domain::Identity remote;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_lost::serialize( archive);
                  CASUAL_SERIALIZE( configuration);
                  CASUAL_SERIALIZE( remote);
               )
            };
            
         } // connection

      } // inbound

      namespace outbound
      {
         namespace state
         {
            using Connection = message::state::basic_connection< casual::configuration::model::gateway::outbound::Connection>;

            namespace pending
            {
               struct Message
               {
                  common::strong::correlation::id correlation;
                  common::strong::ipc::id target;
                  common::strong::file::descriptor::id connection;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( correlation);
                     CASUAL_SERIALIZE( target);
                     CASUAL_SERIALIZE( connection);
                  )
               };

               struct Transaction
               {
                  common::transaction::global::ID gtrid;
                  std::vector< common::strong::file::descriptor::id> connections;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( gtrid);
                     CASUAL_SERIALIZE( connections);
                  )
               };

            } // pending

            struct Pending
            {
               std::vector< pending::Message> messages;
               std::vector< pending::Transaction> transactions;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( messages);
                  CASUAL_SERIALIZE( transactions);
               )
            };

            namespace connection
            {
               struct Identifier
               {
                  common::strong::process::id pid{};
                  common::strong::file::descriptor::id descriptor{};

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( pid);
                     CASUAL_SERIALIZE( descriptor);
                  )
               };
            }

            struct Routing
            {
               std::string name;
               std::vector< connection::Identifier> connections;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( name);
                  CASUAL_SERIALIZE( connections);
               )
            };

            struct Correlation
            {
               std::vector< Routing> services;
               std::vector< Routing> queues;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( services);
                  CASUAL_SERIALIZE( queues);
               )
            };
            
         } // state

         struct State
         {
            std::string alias;
            std::string note;
            platform::size::type order{};

            std::vector< state::Connection> connections;

            state::Pending pending;

            state::Correlation correlation;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( alias);
               CASUAL_SERIALIZE( note);
               CASUAL_SERIALIZE( order);
               CASUAL_SERIALIZE( connections);
               CASUAL_SERIALIZE( pending);
               CASUAL_SERIALIZE( correlation);
            )
         };


         using Connect = common::message::basic_request< common::message::Type::gateway_outbound_connect>;

         namespace configuration::update 
         {
            using base_request = common::message::basic_request< common::message::Type::gateway_outbound_configuration_update_request>;
            struct Request : base_request
            {
               using base_request::base_request;

               platform::size::type order{};
               casual::configuration::model::gateway::outbound::Group model;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_request::serialize( archive);
                  CASUAL_SERIALIZE( order);
                  CASUAL_SERIALIZE( model);
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
         } // reverse

         namespace connection
         {
            using base_lost = common::message::basic_message< common::message::Type::gateway_outbound_connection_lost>;
            struct Lost : base_lost
            {
               Lost() = default;
               Lost( casual::configuration::model::gateway::outbound::Connection configuration, common::domain::Identity remote)
                  : configuration{ std::move( configuration)}, remote{ std::move( remote)} {}

               casual::configuration::model::gateway::outbound::Connection configuration;
               common::domain::Identity remote;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_lost::serialize( archive);
                  CASUAL_SERIALIZE( configuration);
                  CASUAL_SERIALIZE( remote);
               )
            };
            
         } // connection
         
      } // outbound

      //! make sure we specialize `protocol::version_traits` for the correct version.
      namespace protocol
      {   
         template<>
         struct version_traits< domain::disconnect::Request> : version_helper< Version::v1_1> {};

         template<>
         struct version_traits< domain::disconnect::Reply> : version_helper< Version::v1_1> {};

         template<>
         struct version_traits< casual::domain::message::discovery::topology::implicit::Update> : version_helper< Version::v1_2> {};

      } // protocol

   } // gateway::message

   namespace common
   {
      namespace message::reverse
      {
         template<>
         struct type_traits< casual::gateway::message::domain::connect::Request> : detail::type< casual::gateway::message::domain::connect::Reply> {};

         template<>
         struct type_traits< casual::gateway::message::domain::disconnect::Request> : detail::type< casual::gateway::message::domain::disconnect::Reply> {};

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

      namespace serialize::customize::composite
      {

//! customization macro to make it easier to define customization points for all interdomain messages
#define CASUAL_CUSTOMIZATION_POINT_NETWORK( type, statement) \
template< typename A> \
requires common::serialize::archive::network::normalizing_v< A> \
struct Value< type, A>  \
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

         CASUAL_CUSTOMIZATION_POINT_NETWORK( casual::gateway::message::domain::disconnect::Request,
         {
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( execution);
         })

         CASUAL_CUSTOMIZATION_POINT_NETWORK( casual::gateway::message::domain::disconnect::Reply,
         {
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( execution);
         })

         // value
         CASUAL_CUSTOMIZATION_POINT_NETWORK( casual::domain::message::discovery::reply::Queue,
         {
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( name);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( retries);
         })

         // value
         CASUAL_CUSTOMIZATION_POINT_NETWORK( casual::domain::message::discovery::reply::Service,
         {
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( name);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( category);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( transaction);
            // TODO 2.0
            // CASUAL_CUSTOMIZATION_POINT_SERIALIZE( type);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( timeout.duration);
            CASUAL_SERIALIZE_NAME( value.property.hops, "hops");
         })

         CASUAL_CUSTOMIZATION_POINT_NETWORK( casual::domain::message::discovery::Request,
         {
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( execution);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( domain);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( content.services);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( content.queues);
         })

         CASUAL_CUSTOMIZATION_POINT_NETWORK( casual::domain::message::discovery::Reply,
         {
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( execution);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( domain);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( content.services);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( content.queues);
         })
         
         CASUAL_CUSTOMIZATION_POINT_NETWORK( casual::domain::message::discovery::topology::implicit::Update,
         {
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( execution);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( domains);
         })



         CASUAL_CUSTOMIZATION_POINT_NETWORK( common::message::service::call::callee::Request,
         {
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( execution);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( service.name);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( service.timeout.duration);
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
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( service.timeout.duration);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( parent);
            CASUAL_SERIALIZE_NAME( value.trid.xid, "xid");
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( duplex);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( buffer);
         })

         CASUAL_CUSTOMIZATION_POINT_NETWORK( common::message::conversation::connect::Reply,
         {
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( execution);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( code.result);
         })


         CASUAL_CUSTOMIZATION_POINT_NETWORK( common::message::conversation::Disconnect,
         {
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( execution);
         })

         CASUAL_CUSTOMIZATION_POINT_NETWORK( common::message::conversation::callee::Send,
         {
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( execution);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( duplex);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( code.result);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( code.user);
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

         CASUAL_CUSTOMIZATION_POINT_NETWORK( queue::ipc::message::group::enqueue::Request,
         {
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( execution);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( name);
            CASUAL_SERIALIZE_NAME( value.trid.xid, "xid");
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( message);
         })

         CASUAL_CUSTOMIZATION_POINT_NETWORK( queue::ipc::message::group::enqueue::Reply,
         {
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( execution);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( id);
         })
      

         CASUAL_CUSTOMIZATION_POINT_NETWORK( queue::ipc::message::group::dequeue::Request,
         {
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( execution);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( name);
            CASUAL_SERIALIZE_NAME( value.trid.xid, "xid");
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( selector);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( block);
         })


         CASUAL_CUSTOMIZATION_POINT_NETWORK( queue::ipc::message::group::dequeue::Reply,
         {
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( execution);
            CASUAL_CUSTOMIZATION_POINT_SERIALIZE( message);
         })
         
      } // serialize::customize::composite
   } // common
} // casual




