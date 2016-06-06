//!
//! type.h
//!
//! Created on: Jun 14, 2014
//!     Author: Lazan
//!

#ifndef COMMONMESSAGETYPE_H_
#define COMMONMESSAGETYPE_H_

#include "common/platform.h"
#include "common/transaction/id.h"

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
            shutdownd_request,
            shutdownd_reply,
            forward_connect_request,
            forward_connect_reply,
            delay_message,
            inbound_ipc_connect,

            process_spawn_request = 600,
            process_lookup_request,
            process_lookup_reply,

            DOMAIN_BASE = 1000,
            domain_discover_request,
            domain_discover_reply,
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
            service_unadvertise,
            service_name_lookup_request,
            service_name_lookup_reply,
            service_call,
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
            transaction_domain_resource_prepare_request = TRANSACTION_BASE + 300,
            transaction_domain_resource_prepare_reply,
            transaction_domain_resource_commit_request,
            transaction_domain_resource_commit_reply,
            transaction_domain_resource_rollback_request,
            transaction_domain_resource_rollback_reply,

            transaction_resource_involved = TRANSACTION_BASE + 400,
            transaction_domain_resource_involved,

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
            queue_information = QUEUE_BASE + 300,
            queue_queues_information_request,
            queue_queues_information_reply,
            queue_queue_information_request,
            queue_queue_information_reply,
            queue_lookup_request = QUEUE_BASE + 400,
            queue_lookup_reply,
            queue_group_involved = QUEUE_BASE + 500,


            GATEWAY_BASE = 7000,
            gateway_manager_listener_event,
            gateway_manager_tcp_connect,
            gateway_outbound_connect,
            gateway_inbound_connect,
            gateway_worker_connect,
            gateway_worker_disconnect,
            gateway_ipc_connect_request,
            gateway_ipc_connect_reply,
            gateway_service_discover_request,
            gateway_service_discover_reply,




            MOCKUP_BASE = 1000000, // avoid conflict with real messages
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

         template< typename M, message::Type message_type>
         struct type_wrapper : public M
         {
            using M::M;

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
               M::marshal( archive);
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
            struct Request : basic_message< Type::shutdownd_request>
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

            struct Reply : basic_message< Type::shutdownd_reply>
            {
               template< typename ID>
               struct holder_t
               {
                  std::vector< ID> online;
                  std::vector< ID> offline;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     archive & online;
                     archive & offline;
                  })
               };

               holder_t< platform::pid::type> executables;
               holder_t< common::process::Handle> servers;


               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  base_type::marshal( archive);
                  archive & executables;
                  archive & servers;
               })
            };

         } // shutdown



         //
         // Below, some basic message related types that is used by others
         //

         struct Service
         {

            Service() = default;
            Service& operator = (const Service& rhs) = default;



            explicit Service( std::string name, std::uint64_t type, std::uint64_t transaction)
               : name( std::move( name)), type( type), transaction( transaction)
            {}

            Service( std::string name)
               : Service( std::move( name), 0, 0)
            {}

            std::string name;
            std::uint64_t type = 0;
            std::chrono::microseconds timeout = std::chrono::microseconds::zero();
            std::vector< platform::ipc::id::type> traffic_monitors;
            std::uint64_t transaction = 0;

            CASUAL_CONST_CORRECT_MARSHAL(
            {
               archive & name;
               archive & type;
               archive & timeout;
               archive & traffic_monitors;
               archive & transaction;
            })

            friend std::ostream& operator << ( std::ostream& out, const Service& value);
         };

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

            } // spawn






         } // process



         namespace forward
         {
            namespace connect
            {
               using Request = server::connect::basic_request< Type::forward_connect_request>;
               using Reply = server::connect::basic_reply< Type::forward_connect_reply>;
            } // connect

         } // forward

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

            template<>
            struct type_traits< shutdown::Request> : detail::type< shutdown::Reply> {};

            template<>
            struct type_traits< forward::connect::Request> : detail::type< forward::connect::Reply> {};



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
