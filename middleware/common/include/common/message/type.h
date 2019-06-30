//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/platform.h"
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

            process_spawn_request = 600,
            process_lookup_request,
            process_lookup_reply,

            DOMAIN_BASE = 1000,
            domain_scale_executable,
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

            domain_pending_send_connect = DOMAIN_BASE + 300,
            domain_pending_send_request,



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
            service_call = SERVICE_BASE + 100,
            service_reply,
            service_acknowledge,

            service_concurrent_advertise,

            service_conversation_connect_request = SERVICE_BASE + 200,
            service_conversation_connect_reply,
            service_conversation_send,
            service_conversation_disconnect,


            // event messages
            EVENT_BASE = 4000,
            event_subscription_begin,
            event_subscription_end,

            EVENT_DOMAIN_BASE = 4100,
            event_domain_boot_begin = EVENT_DOMAIN_BASE,
            event_domain_boot_end,
            event_domain_shutdown_begin,
            event_domain_shutdown_end,
            event_domain_error,
            event_domain_server_connect,
            event_domain_group,

            event_process_spawn,
            event_process_exit,
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
            transaction_begin_request = TRANSACTION_BASE + 100,
            transaction_begin_reply,
            transaction_commit_request,
            transaction_commit_reply,
            transaction_Rollback_request,
            transaction_rollback_reply,
            transaction_generic_reply,

            transaction_resource_connect_reply = TRANSACTION_BASE + 200,
            transaction_resource_prepare_request,
            transaction_resource_prepare_reply,
            transaction_resource_commit_request,
            transaction_resource_commit_reply,
            transaction_resource_rollback_request,
            transaction_resource_rollback_reply,

            transaction_resource_lookup_request = TRANSACTION_BASE + 300,
            transaction_resource_lookup_reply,

            transaction_resource_involved_request = TRANSACTION_BASE + 400,
            transaction_resource_involved_reply,
            transaction_external_resource_involved,

            transaction_resource_id_request = TRANSACTION_BASE + 500,
            transaction_resource_id_reply,

            // casual queue
            QUEUE_BASE = 6000,
            queue_connect_request,
            queue_connect_reply,
            queue_advertise,
            queue_enqueue_request = QUEUE_BASE + 100,
            queue_enqueue_reply,
            queue_dequeue_request = QUEUE_BASE + 200,
            queue_dequeue_reply,
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

            queue_restore_request = QUEUE_BASE + 600,
            queue_restore_reply,

            GATEWAY_BASE = 7000,
            gateway_outbound_configuration_request,
            gateway_outbound_configuration_reply,
            gateway_outbound_connect,
            gateway_inbound_connect,

            gateway_outbound_connect_done,

            gateway_domain_connect_request = GATEWAY_BASE + 200,
            gateway_domain_connect_reply,
            gateway_domain_discover_request = GATEWAY_BASE + 300,
            gateway_domain_discover_reply,
            gateway_domain_discover_accumulated_reply,
            gateway_domain_advertise,
            gateway_domain_id,




            UNITTEST_BASE = 10000000, // avoid conflict with real messages
            unittest_message,
         };

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

            using base_type = basic_message< message_type>;

            constexpr static Type type() { return message_type;}

            Uuid correlation;

            //! The execution-id
            mutable Uuid execution;

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

         //! Message to "force" exit/termination.
         //! useful in unittest, to force exit on blocking read
         namespace shutdown
         {
            struct Request : basic_message< Type::shutdown_request>
            {
               inline Request() = default;
               inline Request( common::process::Handle process) : process{ std::move( process)} {}

               common::process::Handle process = common::process::handle();
               bool reply = false;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  base_type::serialize( archive);
                  CASUAL_SERIALIZE( process);
                  CASUAL_SERIALIZE( reply);
               })
            };
            static_assert( traits::is_movable< Request>::value, "not movable");

         } // shutdown

         // Below, some basic message related types that is used by others

         template< message::Type type>
         struct basic_request : basic_message< type>
         {
            common::process::Handle process;

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               basic_message< type>::serialize( archive);
               CASUAL_SERIALIZE( process);
            })

         };

         template< message::Type type>
         struct basic_reply : basic_message< type>
         {
            common::process::Handle process;

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               basic_message< type>::serialize( archive);
               CASUAL_SERIALIZE( process);
            })
         };


         namespace server
         {
            template< message::Type type>
            struct basic_id : basic_request< type>
            {
            };

            namespace connect
            {
               template< message::Type type>
               struct basic_request : basic_id< type>
               {
                  std::string path;
                  Uuid identification;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     basic_id< type>::serialize( archive);
                     CASUAL_SERIALIZE( path);
                     CASUAL_SERIALIZE( identification);
                  })
               };

               template< message::Type type>
               struct basic_reply : basic_message< type>
               {
                  enum class Directive : char
                  {
                     start,
                     singleton,
                     shutdown
                  };

                  Directive directive = Directive::start;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     basic_message< type>::serialize( archive);
                     CASUAL_SERIALIZE( directive);
                  })
               };
            } // connect



            template< message::Type type>
            struct basic_disconnect : basic_id< type>
            {

            };

         } // server


         namespace process
         {
            namespace spawn
            {
               struct Request : basic_message< Type::process_spawn_request>
               {
                  std::string executable;
                  std::vector< std::string> arguments;
                  std::vector< std::string> environment;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     basic_message< Type::process_spawn_request>::serialize( archive);
                     CASUAL_SERIALIZE( executable);
                     CASUAL_SERIALIZE( arguments);
                     CASUAL_SERIALIZE( environment);
                  })
               };
               static_assert( traits::is_movable< Request>::value, "not movable");

            } // spawn
         } // process

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
            auto type( T&& message) -> typename type_traits< typename std::decay<T>::type>::reverse_type
            {
               typename type_traits< typename std::decay<T>::type>::reverse_type result;

               result.correlation = message.correlation;
               result.execution = message.execution;

               return result;
            }

         } // reverse
      } // message
   } // common
} // casual


