//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "casual/platform.h"
#include "common/transaction/id.h"
#include "common/service/type.h"

#include "common/serialize/macro.h"


#include <type_traits>

namespace casual
{
   namespace common
   {
      namespace message
      {

         enum class Type : platform::ipc::message::type
         {
            // names that has a explicit assigned value is _pinned_ 
            // and if changed we break interdomain protocol. 

            // message type can't be 0!
            // We use 0 to indicate absent message
            absent_message = 0,

            UTILITY_BASE = 500,
            flush_ipc, // dummy message used to flush queue (into cache)
            poke,
            shutdown_request,
            shutdown_reply,
            delay_message,
            inbound_ipc_connect,

            process_lookup_request = 600, // not pinned
            process_lookup_reply,

            // domain
            DOMAIN_BASE = 1000,
            domain_spawn_entities,
            domain_process_connect_request,
            domain_process_connect_reply,

            domain_process_lookup_request,
            domain_process_lookup_reply,

            domain_process_prepare_shutdown_request,
            domain_process_prepare_shutdown_reply,

            domain_configuration_request = DOMAIN_BASE + 200,
            domain_configuration_reply,
            domain_server_configuration_request,
            domain_server_configuration_reply,

            domain_pending_message_connect_request = DOMAIN_BASE + 300,
            domain_pending_message_connect_reply,
            domain_pending_message_send_request,

            domain_instance_global_state_request = DOMAIN_BASE + 400,
            domain_instance_global_state_reply,


            // Server
            SERVER_BASE = 2000,
            server_connect_request,
            server_connect_reply,
            server_disconnect,
            server_ping_request,
            server_ping_reply,

            // Service
            SERVICE_BASE = 3000,
            service_advertise,
            service_name_lookup_request,
            service_name_lookup_reply,
            service_name_lookup_discard_request,
            service_name_lookup_discard_reply,
            service_call   = 3100,
            service_reply  = 3101,
            service_acknowledge,

            service_concurrent_advertise,

            service_conversation_connect_request = 3200,
            service_conversation_connect_reply   = 3201,
            service_conversation_send            = 3202,
            service_conversation_disconnect      = 3203,


            // event messages
            EVENT_BASE = 4000,
            event_subscription_begin,
            event_subscription_end,
            event_idle,


            EVENT_DOMAIN_BASE = 4100,
            
            event_task = EVENT_DOMAIN_BASE,
            event_sub_task,
            event_error,

            event_process_spawn,
            event_process_exit,

            // internal domain events
            event_domain_information,

            EVENT_DOMAIN_BASE_END,

            EVENT_SERVICE_BASE = 4200,
            event_service_call = EVENT_SERVICE_BASE,
            event_service_calls,
            EVENT_SERVICE_BASE_END,
            
            EVENT_BASE_END,



            // Transaction
            TRANSACTION_BASE = 5000,
            transaction_client_connect_request,
            transaction_client_connect_reply,
            transaction_manager_connect_request,
            transaction_manager_connect_reply,
            transaction_manager_configuration,
            transaction_manager_ready,

            transaction_resource_proxy_configuration_request = TRANSACTION_BASE + 20,
            transaction_resource_proxy_configuration_reply,
            transaction_resource_proxy_ready,

            transaction_begin_request = TRANSACTION_BASE + 100,
            transaction_begin_reply,
            transaction_commit_request,
            transaction_commit_reply,
            transaction_Rollback_request,
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
            queue_group_connect_request,
            queue_group_connect_reply,
            queue_forward_configuration_request,
            queue_forward_configuration_reply,
            queue_forward_state_request,
            queue_forward_state_reply,
            queue_advertise,
            queue_enqueue_request  = 6100,
            queue_enqueue_reply    = 6101,
            queue_dequeue_request  = 6200,
            queue_dequeue_reply    = 6201,
            queue_dequeue_forget_request,
            queue_dequeue_forget_reply,

            queue_peek_information_request =  QUEUE_BASE + 300,
            queue_peek_information_reply,
            queue_peek_messages_request,
            queue_peek_messages_reply,

            queue_information = QUEUE_BASE + 400,
            queue_queues_information_request,
            queue_queues_information_reply,
            queue_queue_information_request,
            queue_queue_information_reply,

            queue_lookup_request = QUEUE_BASE + 500,
            queue_lookup_reply,
            queue_lookup_discard_request,
            queue_lookup_discard_reply,

            queue_restore_request = QUEUE_BASE + 600,
            queue_restore_reply,
            queue_clear_request,
            queue_clear_reply,
            queue_messages_remove_request,
            queue_messages_remove_reply,

            queue_metric_reset_request = QUEUE_BASE + 700,
            queue_metric_reset_reply,
            
            // gateway
            GATEWAY_BASE = 7000,
            gateway_outbound_configuration_request = GATEWAY_BASE,
            gateway_outbound_configuration_reply, 
            gateway_outbound_connect, 
            gateway_inbound_connect, 

            gateway_outbound_connect_done,

            gateway_outbound_rediscover_request,
            gateway_outbound_rediscover_reply, 


            gateway_domain_connect_request  = 7200,
            gateway_domain_connect_reply    = 7201,
            gateway_domain_discover_request = 7300,
            gateway_domain_discover_reply   = 7301,
            gateway_domain_discover_accumulated_reply,
            gateway_domain_advertise,
            gateway_domain_id,

            // signals as messages, used to postpone and normalize handling of signals
            SIGNAL_BASE = 8000,
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

            UNITTEST_BASE = 10000000, // avoid conflict with real messages
            unittest_message,
         };

         inline std::ostream& operator << ( std::ostream& out, Type value) { return out << cast::underlying( value);}

         //! Deduce witch type of message it is.
         //! @{
         template< typename M>
         constexpr Type type( const M& message)
         {
            return message.type();
         }
         //! @}


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

            Uuid correlation;

            //! The execution-id
            mutable Uuid execution;

            constexpr friend bool operator == ( const basic_message&, message::Type rhs) { return basic_message::type() == rhs;}
            constexpr friend bool operator == ( message::Type lhs, const basic_message&) { return basic_message::type() == lhs;}

            constexpr friend bool operator == ( const basic_message& lhs, const Uuid& rhs) { return lhs.correlation == rhs;}
            constexpr friend bool operator == ( const Uuid& lhs, const basic_message& rhs) { return rhs == lhs;}


            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               // correlation is part of ipc::message::Complete, and is
               // handled by the ipc-abstraction (marshaled 'on the side')
               CASUAL_SERIALIZE( execution);
            })
         };

         //! Wraps a message with basic_message
         template< typename Message, message::Type message_type>
         struct type_wrapper : Message, basic_message< message_type>
         {
            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               basic_message< message_type>::serialize( archive);
               Message::serialize( archive);
            })
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
            {
               CASUAL_SERIALIZE( start);
               CASUAL_SERIALIZE( end);
            })
         };

         template< typename M>
         auto correlation( M& message) -> decltype( message.correlation)
         {
            return message.correlation;
         }

         // Below, some basic message related types that is used by others

         template< message::Type type>
         struct basic_request : basic_message< type>
         {
            basic_request() = default;
            inline basic_request( common::process::Handle process) : process{ std::move( process)} {}

            common::process::Handle process;

            friend bool operator == ( const basic_request& lhs, strong::process::id rhs) { return lhs.process == rhs;}
            friend bool operator == ( strong::process::id lhs, const basic_request& rhs) { return rhs == lhs;}

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               basic_message< type>::serialize( archive);
               CASUAL_SERIALIZE( process);
            })

         };

         template< message::Type type>
         struct basic_reply : basic_message< type>
         {
            basic_reply() = default;
            inline basic_reply( common::process::Handle process) : process{ std::move( process)} {}

            common::process::Handle process;

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               basic_message< type>::serialize( archive);
               CASUAL_SERIALIZE( process);
            })
         };

         //! Message to "force" exit/termination.
         //! useful in unittest, to force exit on blocking read
         namespace shutdown
         {
            using base_request = basic_request< Type::shutdown_request>;
            struct Request : base_request
            {
               using base_request::base_request;
               bool reply = false;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  base_request::serialize( archive);
                  CASUAL_SERIALIZE( reply);
               })
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

            template< typename T, typename... Ts>
            auto type( T&& message, Ts&&... ts)
            {
               typename type_traits< typename std::decay<T>::type>::reverse_type result{ std::forward< Ts>( ts)...};

               result.correlation = message.correlation;
               result.execution = message.execution;

               return result;
            }

         } // reverse
      } // message
   } // common
} // casual


