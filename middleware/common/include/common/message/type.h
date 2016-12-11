//!
//! casual
//!

#ifndef COMMONMESSAGETYPE_H_
#define COMMONMESSAGETYPE_H_

#include "common/platform.h"
#include "common/transaction/id.h"
#include "common/service/type.h"

#include "common/marshal/marshal.h"


#include <type_traits>

namespace casual
{
   namespace common
   {
      namespace message
      {
         enum class Type : platform::ipc::message::type
         {
            //
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
            domain_process_termination_registration,
            domain_process_termination_event,
            domain_process_lookup_request,
            domain_process_lookup_reply,
            domain_configuration_transaction_resource_request = DOMAIN_BASE + 100,
            domain_configuration_transaction_resource_reply,
            domain_configuration_gateway_request,
            domain_configuration_gateway_reply,
            domain_configuration_queue_request,
            domain_configuration_queue_reply,

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
            service_call = SERVICE_BASE + 100,
            service_reply,
            service_acknowledge,

            // Monitor
            TRAFFICMONITOR_BASE = 4000,
            traffic_monitor_connect_request,
            traffic_monitor_connect_reply,
            traffic_monitor_disconnect,
            traffic_event,

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

            transaction_resurce_connect_reply = TRANSACTION_BASE + 200,
            transaction_resource_prepare_request,
            transaction_resource_prepare_reply,
            transaction_resource_commit_request,
            transaction_resource_commit_reply,
            transaction_resource_rollback_request,
            transaction_resource_rollback_reply,


            transaction_resource_involved = TRANSACTION_BASE + 400,
            transaction_external_resource_involved,

            transaction_resource_id_request = TRANSACTION_BASE + 500,
            transaction_resource_id_reply,

            // casual queue
            QUEUE_BASE = 6000,
            queue_connect_request,
            queue_connect_reply,
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
            gateway_manager_listener_event,
            gateway_manager_tcp_connect,
            gateway_outbound_configuration_request,
            gateway_outbound_configuration_reply,
            gateway_outbound_connect,
            gateway_inbound_connect,
            gateway_worker_connect,
            gateway_worker_disconnect,
            gateway_ipc_connect_request,
            gateway_ipc_connect_reply,
            gateway_domain_discover_request,
            gateway_domain_discover_reply,
            gateway_domain_discover_accumulated_reply,
            gateway_domain_discover_internal_coordination,
            gateway_domain_advertise,
            gateway_domain_id,

            // Innterdomain messages, part of gateway
            INTERDOMAIN_BASE = 8000,
            interdomain_domain_discover_request,
            interdomain_domain_discover_reply,
            interdomain_service_call = INTERDOMAIN_BASE + 100,
            interdomain_service_reply,
            interdomain_transaction_resource_prepare_request = INTERDOMAIN_BASE + 300,
            interdomain_transaction_resource_prepare_reply,
            interdomain_transaction_resource_commit_request,
            interdomain_transaction_resource_commit_reply,
            interdomain_transaction_resource_rollback_request,
            interdomain_transaction_resource_rollback_reply,
            interdomain_queue_enqueue_request = INTERDOMAIN_BASE + 400,
            interdomain_queue_enqueue_reply,
            interdomain_queue_dequeue_request,
            interdomain_queue_dequeue_reply,




            MOCKUP_BASE = 10000000, // avoid conflict with real messages
            mockup_disconnect,
            mockup_clear,
         };

         //!
         //! Deduce witch type of message it is.
         //!
         template< typename M>
         constexpr Type type( const M& message)
         {
            return message.type();
         }


         namespace convert
         {
            using underlying_type = typename std::underlying_type< Type>::type;

            constexpr Type type( underlying_type type) { return static_cast< Type>( type);}
            constexpr underlying_type type( Type type) { return static_cast< underlying_type>( type);}

         } // convert


         template< message::Type message_type>
         struct basic_message
         {

            using base_type = basic_message< message_type>;


            constexpr static Type type() { return message_type;}

            Uuid correlation;

            //!
            //! The execution-id
            //!
            mutable Uuid execution;

            CASUAL_CONST_CORRECT_MARSHAL(
            {

               //
               // correlation is part of ipc::message::Complete, and is
               // handled by the ipc-abstraction (marshaled 'on the side')
               //

               archive & execution;
            })
         };


         namespace flush
         {
            using IPC = basic_message< Type::flush_ipc>;

         } // flush


         struct Statistics
         {
            platform::time_point start;
            platform::time_point end;

            CASUAL_CONST_CORRECT_MARSHAL(
            {
               archive & start;
               archive & end;
            })

            friend std::ostream& operator << ( std::ostream& out, const Statistics& message);

         };



         template< typename M>
         auto correlation( M& message) -> decltype( message.correlation)
         {
            return message.correlation;
         }


         //!
         //! Message to "force" exit/termination.
         //! useful in unittest, to force exit on blocking read
         //!
         namespace shutdown
         {
            struct Request : basic_message< Type::shutdown_request>
            {
               inline Request() = default;
               inline Request( common::process::Handle process) : process{ std::move( process)} {}

               common::process::Handle process;
               bool reply = false;

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  base_type::marshal( archive);
                  archive & process;
                  archive & reply;
               })
            };
            static_assert( traits::is_movable< Request>::value, "not movable");

         } // shutdown


         //
         // Below, some basic message related types that is used by others
         //


         namespace server
         {

            template< message::Type type>
            struct basic_id : basic_message< type>
            {
               common::process::Handle process;

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  basic_message< type>::marshal( archive);
                  archive & process;
               })
            };

            namespace connect
            {

               template< message::Type type>
               struct basic_request : basic_id< type>
               {
                  std::string path;
                  Uuid identification;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     basic_id< type>::marshal( archive);
                     archive & path;
                     archive & identification;
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

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     basic_message< type>::marshal( archive);
                     archive & directive;
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

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     basic_message< Type::process_spawn_request>::marshal( archive);
                     archive & executable;
                     archive & arguments;
                     archive & environment;
                  })
               };
               static_assert( traits::is_movable< Request>::value, "not movable");

            } // spawn
         } // process

         namespace reverse
         {


            //!
            //! declaration of helper traits to get the
            //! "reverse type". Normally to get the Reply-type
            //! from a Request-type, and vice versa.
            //!
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

#endif // TYPE_H_
