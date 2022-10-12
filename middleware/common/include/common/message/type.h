//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "casual/platform.h"

#include "common/strong/type.h"
#include "common/serialize/macro.h"
#include "common/execution.h"
#include "common/process.h"

#include <type_traits>

namespace casual
{
   namespace common::message
   {
      enum class Type : platform::ipc::message::type
      {
         // names that has a explicit assigned value is _pinned_ 
         // and if changed we break interdomain protocol. 

         // message type can't be 0!
         // We use 0 to indicate absent message
         absent_message = 0,

         UTILITY_BASE = 500,
         flush_ipc = UTILITY_BASE, // dummy message used to flush queue (into cache)
         poke,
         shutdown_request,
         shutdown_reply,
         delay_message,
         inbound_ipc_connect,

         process_lookup_request = 600, // not pinned
         process_lookup_reply,

         // domain
         DOMAIN_BASE = 1000,

         domain_process_connect_request = DOMAIN_BASE,
         domain_process_connect_reply,

         domain_process_prepare_shutdown_request,
         domain_process_prepare_shutdown_reply,

         domain_manager_shutdown_request,
         domain_manager_shutdown_reply,

         domain_process_lookup_request,
         domain_process_lookup_reply,

         domain_process_information_request,
         domain_process_information_reply,

         domain_configuration_request = DOMAIN_BASE + 200,
         domain_configuration_reply,
         domain_server_configuration_request,
         domain_server_configuration_reply,

         domain_pending_message_connect_request = DOMAIN_BASE + 300,
         domain_pending_message_connect_reply,
         domain_pending_message_send_request,

         domain_instance_global_state_request = DOMAIN_BASE + 400,
         domain_instance_global_state_reply,

         domain_discovery_api_provider_registration_request = DOMAIN_BASE + 500,
         domain_discovery_api_provider_registration_reply,
         domain_discovery_api_request,
         domain_discovery_api_reply,
         domain_discovery_api_rediscovery_request,
         domain_discovery_api_rediscovery_reply,

         // the following is part of interdomain protocol and defined (and pinned) under gateway
         // domain_discovery_request,
         // domain_discovery_reply,

         domain_discovery_needs_request,
         domain_discovery_needs_reply,

         domain_discovery_known_request,
         domain_discovery_known_reply,

         // Server
         SERVER_BASE = 2000,
         server_connect_request = SERVER_BASE,
         server_connect_reply,
         server_disconnect,
         server_ping_request,
         server_ping_reply,

         // Service
         SERVICE_BASE = 3000,
         service_advertise = SERVICE_BASE,
         service_name_lookup_request,
         service_name_lookup_reply,
         service_name_lookup_discard_request,
         service_name_lookup_discard_reply,
         service_call   = 3100,
         service_reply  = 3101,
         service_acknowledge,

         service_concurrent_advertise,

         // The following cant be used.
         // 3200
         // 3201
         // 3202
         // 3203

         conversation_connect_request = 3210,
         conversation_connect_reply   = 3211,
         conversation_send            = 3212,
         conversation_disconnect      = 3213,


         // event messages
         EVENT_BASE = 4000,
         event_subscription_begin = EVENT_BASE,
         event_subscription_end,
         event_idle,


         EVENT_DOMAIN_BASE = 4100,
         
         event_task = EVENT_DOMAIN_BASE,
         event_sub_task,
         event_error,
         event_notification,

         event_process_spawn,
         // sent when a 'process' is done with configuration of it self.
         event_process_configured,
         event_process_exit,
         event_process_assassination_contract,

         // internal domain events
         event_domain_information,

         EVENT_DOMAIN_BASE_END,

         EVENT_SERVICE_BASE = 4200,
         event_service_call = EVENT_SERVICE_BASE,
         event_service_calls,
         EVENT_SERVICE_BASE_END,
         
         EVENT_BASE_END = EVENT_SERVICE_BASE_END,



         // Transaction
         TRANSACTION_BASE = 5000,
         transaction_client_connect_request = TRANSACTION_BASE,
         transaction_client_connect_reply,
         transaction_manager_connect_request,
         transaction_manager_connect_reply,
         transaction_manager_configuration,
         transaction_manager_ready,

         transaction_resource_proxy_configuration_request = TRANSACTION_BASE + 20,
         transaction_resource_proxy_configuration_reply,
         transaction_resource_proxy_ready,

         transaction_configuration_alias_request = TRANSACTION_BASE + 40,
         transaction_configuration_alias_reply,

         transaction_begin_request = TRANSACTION_BASE + 100,
         transaction_begin_reply,
         transaction_commit_request,
         transaction_commit_reply,
         transaction_rollback_request,
         transaction_rollback_reply,
         transaction_generic_reply,

         // pinned message types,
         transaction_resource_prepare_request  = 5201,
         transaction_resource_prepare_reply    = 5202,
         transaction_resource_commit_request   = 5203,
         transaction_resource_commit_reply     = 5204,
         transaction_resource_rollback_request = 5205,   
         transaction_resource_rollback_reply   = 5206,

         transaction_resource_lookup_request = TRANSACTION_BASE + 300,
         transaction_resource_lookup_reply,

         transaction_resource_involved_request = TRANSACTION_BASE + 400,
         transaction_resource_involved_reply,
         transaction_external_resource_involved,

         transaction_resource_id_request = TRANSACTION_BASE + 500,
         transaction_resource_id_reply,

         // casual queue
         QUEUE_BASE = 6000,

         queue_manager_queue_advertise = QUEUE_BASE,
         queue_manager_queue_lookup_request,
         queue_manager_queue_lookup_reply,
         queue_manager_queue_lookup_discard_request,
         queue_manager_queue_lookup_discard_reply,

         // pinned messages
         queue_group_enqueue_request  = 6100,
         queue_group_enqueue_reply    = 6101,
         queue_group_dequeue_request  = 6200,
         queue_group_dequeue_reply    = 6201,
         queue_group_dequeue_forget_request, // might be part of interdomian protocol?
         queue_group_dequeue_forget_reply, // might be part of interdomian protocol?

         queue_group_connect = QUEUE_BASE + 300,
         queue_group_configuration_update_request,
         queue_group_configuration_update_reply,
         queue_group_state_request,
         queue_group_state_reply,
         queue_group_message_meta_request = QUEUE_BASE + 310,
         queue_group_message_meta_reply,
         queue_group_message_meta_peek_request,
         queue_group_message_meta_peek_reply,
         queue_group_message_peek_request = QUEUE_BASE + 320,
         queue_group_message_peek_reply,
         queue_group_message_remove_request = QUEUE_BASE + 330,
         queue_group_message_remove_reply,
         queue_group_message_recovery_request,
         queue_group_message_recovery_reply,

         queue_group_queue_restore_request = QUEUE_BASE + 340,
         queue_group_queue_restore_reply,
         queue_group_queue_clear_request,
         queue_group_queue_clear_reply,

         queue_group_metric_reset_request = QUEUE_BASE + 350,
         queue_group_metric_reset_reply,

         queue_forward_group_connect = QUEUE_BASE + 400,
         queue_forward_group_configuration_update_request,
         queue_forward_group_configuration_update_reply,
         queue_forward_group_state_request,
         queue_forward_group_state_reply,

         
         // gateway
         GATEWAY_BASE = 7000,

         // Inbounds
         gateway_inbound_connect = GATEWAY_BASE,

         // sent from gateway-manager - at least after inbound_connect
         gateway_inbound_configuration_update_request,
         gateway_inbound_configuration_update_reply,

         gateway_inbound_state_request,
         gateway_inbound_state_reply,
         gateway_inbound_connection_lost,

         // reverse inbounds have different state than 'regular' (no listeners)
         gateway_reverse_inbound_state_request,
         gateway_reverse_inbound_state_reply, 

         // Outbounds
         gateway_outbound_connect,

         // sent from gateway-manager - at least after outbound_connect
         gateway_outbound_configuration_update_request, 
         gateway_outbound_configuration_update_reply,

         gateway_outbound_state_request,
         gateway_outbound_state_reply,
         gateway_outbound_connection_lost,

         // reverse outbounds have different state than 'regular' (listeners)
         gateway_reverse_outbound_state_request,
         gateway_reverse_outbound_state_reply, 

         // interdomain 

         gateway_domain_connect_request    = 7200,
         gateway_domain_connect_reply      = 7201,
         gateway_domain_disconnect_request = 7202,  // 1.1
         gateway_domain_disconnect_reply   = 7203,  // 1.1

         // part of domain-discovery, but we need to keep the pinned values.         
         domain_discovery_request   = 7300,
         domain_discovery_reply     = 7301,
         domain_discovery_topology_implicit_update = 7302, // 1.2

         domain_discovery_internal_request,
         domain_discovery_internal_reply,

         //! sent to _discovery_ when current domain gets a new outbound connection
         domain_discovery_topology_direct_update,

         //! sent from the connector when the logical connected is established
         gateway_domain_connected,


         CONFIGURATION_BASE = 8000,
         configuration_request = CONFIGURATION_BASE,
         configuration_reply,
         configuration_supplier_registration,
         configuration_update_request,
         configuration_update_reply,
         configuration_put_request,
         configuration_put_reply,


         // signals as messages, used to postpone and normalize handling of signals
         SIGNAL_BASE = 9000,
         signal_timeout = SIGNAL_BASE,
         signal_hangup,

         CLI_BASE = 10000,
         cli_pipe_done = CLI_BASE,
         cli_payload,
         cli_queue_message_id,
         
         cli_pipe_error_fatal = CLI_BASE + 100,

         cli_transaction_directive = CLI_BASE + 200,
         cli_transaction_directive_terminated,
         cli_transaction_associated,
         cli_transaction_finalize,
         cli_transaction_propagate,

         // internal "troubleshooting" messages
         INTERNAL_BASE  = 11000,
         internal_dump_state = INTERNAL_BASE,
         internal_configure_log,

         UNITTEST_BASE = 10000000, // avoid conflict with real messages
         unittest_message = UNITTEST_BASE,
      };

      std::string_view description( Type value) noexcept;

      //! Deduce witch type of message it is.
      template< typename M>
      constexpr Type type( const M& message)
      {
         return message.type();
      }


      namespace convert
      {
         using underlying_type = std::underlying_type_t< message::Type>;

         constexpr auto type( underlying_type type) { return static_cast< Type>( type);}
         constexpr auto type( Type type) { return cast::underlying( type);}

      } // convert


      template< message::Type message_type>
      struct basic_message
      {
         constexpr static Type type() { return message_type;}

         strong::correlation::id correlation;

         //! The execution-id
         mutable strong::execution::id execution;

         constexpr friend bool operator == ( const basic_message& lhs, message::Type rhs) { return message::type( lhs) == rhs;}
         constexpr friend bool operator == ( message::Type lhs, const basic_message& rhs) { return message::type( rhs) == lhs;}

         constexpr friend bool operator == ( const basic_message& lhs, const strong::correlation::id& rhs) { return lhs.correlation == rhs;}
         constexpr friend bool operator == ( const strong::correlation::id& lhs, const basic_message& rhs) { return rhs == lhs;}


         CASUAL_CONST_CORRECT_SERIALIZE(
            // correlation is part of ipc::message::Complete, and is
            // handled by the ipc-abstraction (marshaled 'on the side')
            CASUAL_SERIALIZE( execution);
         )
      };


      namespace flush
      {
         using IPC = basic_message< Type::flush_ipc>;

      } // flush


      struct Statistics
      {
         platform::time::point::type start;
         platform::time::point::type end;

         CASUAL_CONST_CORRECT_SERIALIZE(
            CASUAL_SERIALIZE( start);
            CASUAL_SERIALIZE( end);
         )
      };

      // Below, some basic message related types that is used by others

      template< message::Type type>
      struct basic_process : basic_message< type>
      {
         basic_process() = default;
         inline basic_process( common::process::Handle process) : process{ std::move( process)} {}

         common::process::Handle process;

         friend bool operator == ( const basic_process& lhs, strong::process::id rhs) { return lhs.process == rhs;}
         friend bool operator == ( strong::process::id lhs, const basic_process& rhs) { return rhs == lhs;}

         CASUAL_CONST_CORRECT_SERIALIZE(
            basic_message< type>::serialize( archive);
            CASUAL_SERIALIZE( process);
         )

      };

      template< message::Type type>
      using basic_request = basic_process< type>;

      template< message::Type type>
      using basic_reply = basic_message< type>;


      //! Message to politely ask a 'server' to exit/termination.
      namespace shutdown
      {
         using base_request = basic_request< Type::shutdown_request>;
         struct Request : base_request
         {
            using base_request::base_request;
         };

      } // shutdown

      namespace reverse
      {
         //! declaration of helper traits to get the
         //! "reverse type". Normally to get the Reply-type
         //! from a Request-type, and vice versa.
         template< typename T>
         struct type_traits;

         namespace detail
         {
            template< typename R>
            struct type
            {
               using reverse_type = R;

            };
         } // detail

         template< typename T>
         using type_t = typename type_traits< std::decay_t< T>>::reverse_type;

         template< typename T, typename... Ts>
         auto type( T&& message, Ts&&... ts)
         {
            reverse::type_t< T> result{ std::forward< Ts>( ts)...};

            result.correlation = message.correlation;
            result.execution = message.execution;

            return result;
         }

      } // reverse

   } // common::message
} // casual


