//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MESSAGE_DOMAIN_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MESSAGE_DOMAIN_H_

#include "common/message/type.h"
#include "common/domain.h"

namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace domain
         {
            namespace scale
            {
               struct Executable : common::message::basic_message< common::message::Type::domain_scale_executable>
               {
                  Executable() = default;
                  Executable( std::initializer_list< std::size_t> executables) : executables{ std::move( executables)} {}

                  std::vector< std::size_t> executables;

                  CASUAL_CONST_CORRECT_MARSHAL({
                     base_type::marshal( archive);
                     archive & executables;
                  })

                  friend std::ostream& operator << ( std::ostream& out, const Executable& value);
               };
               static_assert( traits::is_movable< Executable>::value, "not movable");

            } // scale



            namespace process
            {
               namespace connect
               {
                  struct Request : basic_message< Type::domain_process_connect_request>
                  {
                     common::process::Handle process;
                     Uuid identification;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_type::marshal( archive);
                        archive & process;
                        archive & identification;
                     })
                  };
                  static_assert( traits::is_movable< Request>::value, "not movable");

                  struct Reply : basic_message< Type::domain_process_connect_reply>
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
                        base_type::marshal( archive);
                        archive & directive;
                     })
                  };
                  static_assert( traits::is_movable< Reply>::value, "not movable");

               } // connect



               namespace termination
               {
                  using Registration = server::basic_id< Type::domain_process_termination_registration>;

                  using base_event = basic_message< Type::domain_process_termination_event>;

                  struct Event : base_event
                  {
                     Event() = default;
                     Event( common::process::lifetime::Exit death) : death{ std::move( death)} {}

                     common::process::lifetime::Exit death;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_event::marshal( archive);
                        archive & death;
                     })

                     friend std::ostream& operator << ( std::ostream& out, const Event& value);
                  };
                  static_assert( traits::is_movable< Event>::value, "not movable");

               } // termination

               namespace lookup
               {
                  using base_reqeust = server::basic_id< Type::domain_process_lookup_request>;
                  struct Request : base_reqeust
                  {
                     enum class Directive : char
                     {
                        wait,
                        direct
                     };

                     Uuid identification;
                     platform::pid::type pid = 0;
                     Directive directive = Directive::wait;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_reqeust::marshal( archive);
                        archive & identification;
                        archive & pid;
                        archive & directive;
                     })

                     friend std::ostream& operator << ( std::ostream& out, Directive value);
                     friend std::ostream& operator << ( std::ostream& out, const Request& value);
                  };
                  static_assert( traits::is_movable< Request>::value, "not movable");

                  using base_reply = server::basic_id< Type::domain_process_lookup_reply>;

                  struct Reply : base_reply
                  {
                     Uuid identification;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_reply::marshal( archive);
                        archive & identification;
                     })
                  };
                  static_assert( traits::is_movable< Reply>::value, "not movable");

               } // lookup


            } // process

            namespace configuration
            {
               namespace transaction
               {

                  struct Resource
                  {
                     Resource() = default;
                     Resource( std::function<void(Resource&)> initializer)
                     {
                        initializer( *this);
                     }

                     std::size_t instances = 0;
                     std::size_t id = 0;
                     std::string key;
                     std::string openinfo;
                     std::string closeinfo;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        archive & instances;
                        archive & id;
                        archive & key;
                        archive & openinfo;
                        archive & closeinfo;
                     })

                     friend std::ostream& operator << ( std::ostream& out, const Resource& value);
                  };
                  static_assert( traits::is_movable< Resource>::value, "not movable");

                  namespace resource
                  {
                     using base_request = server::basic_id< Type::domain_configuration_transaction_resource_request>;
                     struct Request : base_request
                     {
                        enum class Scope : char
                        {
                           specific,
                           all,
                        };

                        Scope scope = Scope::specific;

                        CASUAL_CONST_CORRECT_MARSHAL(
                        {
                           base_request::marshal( archive);
                           archive & scope;
                        })
                     };
                     static_assert( traits::is_movable< Request>::value, "not movable");

                     using base_reply = server::basic_id< Type::domain_configuration_transaction_resource_reply>;
                     struct Reply : base_reply
                     {
                        std::vector< Resource> resources;

                        CASUAL_CONST_CORRECT_MARSHAL(
                        {
                           base_reply::marshal( archive);
                           archive & resources;
                        })

                        friend std::ostream& operator << ( std::ostream& out, const Reply& value);
                     };
                     static_assert( traits::is_movable< Reply>::value, "not movable");


                  } // Resource

               } // transaction


               namespace gateway
               {
                  struct Listener
                  {
                     std::string address;

                     CASUAL_CONST_CORRECT_MARSHAL
                     (
                        archive &  address;
                     )

                     friend std::ostream& operator << ( std::ostream& out, const Listener& value);
                  };



                  struct Connection
                  {
                     enum class Type : char
                     {
                        tcp,
                        ipc,
                     };

                     std::string name;
                     std::string address;
                     Type type = Type::tcp;
                     bool restart = false;
                     std::vector< std::string> services;
                     std::vector< std::string> queues;

                     CASUAL_CONST_CORRECT_MARSHAL
                     (
                        archive & name;
                        archive & type;
                        archive & address;
                        archive & restart;
                        archive & services;
                        archive & queues;
                     )

                     friend std::ostream& operator << ( std::ostream& out, Type value);
                     friend std::ostream& operator << ( std::ostream& out, const Connection& value);
                  };
                  static_assert( traits::is_movable< Connection>::value, "not movable");

                  using base_request = server::basic_id< Type::domain_configuration_gateway_request>;
                  struct Request : base_request
                  {

                  };
                  static_assert( traits::is_movable< Request>::value, "not movable");

                  using base_reply = server::basic_id< Type::domain_configuration_gateway_reply>;
                  struct Reply : base_reply
                  {
                     std::vector< Listener> listeners;
                     std::vector< Connection> connections;

                     CASUAL_CONST_CORRECT_MARSHAL
                     (
                        base_reply::marshal( archive);
                        archive & listeners;
                        archive & connections;
                     )

                     friend std::ostream& operator << ( std::ostream& out, const Reply& value);
                  };
                  static_assert( traits::is_movable< Reply>::value, "not movable");


               } // gateway

               namespace queue
               {
                  struct Queue
                  {
                     std::string name;
                     std::size_t retries = 0;
                     std::string note;

                     CASUAL_CONST_CORRECT_MARSHAL
                     (
                        archive & name;
                        archive & retries;
                        archive & note;
                     )

                     friend std::ostream& operator << ( std::ostream& out, const Queue& value);
                  };

                  struct Group
                  {
                     std::string name;
                     std::string queuebase;
                     std::string note;
                     std::vector< Queue> queues;

                     CASUAL_CONST_CORRECT_MARSHAL
                     (
                        archive & name;
                        archive & queuebase;
                        archive & note;
                        archive & queues;
                     )

                     friend std::ostream& operator << ( std::ostream& out, const Group& value);
                  }; // Group


                  using base_request = server::basic_id< Type::domain_configuration_queue_request>;
                  struct Request : base_request
                  {

                  };

                  using base_reply = server::basic_id< Type::domain_configuration_queue_reply>;
                  struct Reply : base_reply
                  {
                     std::vector< Group> groups;

                     CASUAL_CONST_CORRECT_MARSHAL
                     (
                        base_reply::marshal( archive);
                        archive & groups;
                     )

                     friend std::ostream& operator << ( std::ostream& out, const Reply& value);
                  };
                  static_assert( traits::is_movable< Reply>::value, "not movable");

               } // queue

            } // configuration

         } // domain

         namespace reverse
         {
            template<>
            struct type_traits< domain::process::lookup::Request> : detail::type< domain::process::lookup::Reply> {};

            template<>
            struct type_traits< domain::process::connect::Request> : detail::type< domain::process::connect::Reply> {};

            template<>
            struct type_traits< domain::configuration::transaction::resource::Request> : detail::type< domain::configuration::transaction::resource::Reply> {};

            template<>
            struct type_traits< domain::configuration::gateway::Request> : detail::type< domain::configuration::gateway::Reply> {};

            template<>
            struct type_traits< domain::configuration::queue::Request> : detail::type< domain::configuration::queue::Reply> {};


         } // reverse

      } // message
   } // common

} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MESSAGE_DOMAIN_H_
