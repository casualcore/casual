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
            namespace configuration
            {
               namespace transaction
               {
                  struct Resource
                  {
                     Resource() = default;
                     inline Resource( std::function< void(Resource&)> foreign) { foreign( *this);}

                     std::string name;
                     std::string key;
                     std::size_t instances = 0;
                     std::string note;

                     std::string openinfo;
                     std::string closeinfo;

                     CASUAL_CONST_CORRECT_MARSHAL(
                        archive & name;
                        archive & key;
                        archive & instances;
                        archive & note;
                        archive & openinfo;
                        archive & closeinfo;
                     )
                  };

                  struct Manager
                  {
                     std::string log;
                     std::vector< Resource> resources;

                     CASUAL_CONST_CORRECT_MARSHAL
                     (
                        archive & log;
                        archive & resources;
                     )
                  };

               } // transaction



               namespace gateway
               {
                  struct Listener
                  {
                     std::string address;

                     CASUAL_CONST_CORRECT_MARSHAL
                     (
                        archive & address;
                     )
                  };

                  struct Connection
                  {
                     enum class Type : short
                     {
                        ipc,
                        tcp
                     };
                     Type type = Type::tcp;
                     bool restart = true;
                     std::string address;
                     std::string note;
                     std::vector< std::string> services;
                     std::vector< std::string> queues;

                     CASUAL_CONST_CORRECT_MARSHAL
                     (
                        archive & type;
                        archive & restart;
                        archive & address;
                        archive & note;
                        archive & services;
                        archive & queues;
                     )
                  };

                  struct Manager
                  {
                     std::vector< Listener> listeners;
                     std::vector< Connection> connections;

                     CASUAL_CONST_CORRECT_MARSHAL
                     (
                        archive & listeners;
                        archive & connections;
                     )
                  };

               } // gateway



               namespace queue
               {
                  struct Queue
                  {
                     Queue() = default;
                     inline Queue( std::function< void(Queue&)> foreign) { foreign( *this);}

                     std::string name;
                     std::size_t retries = 0;
                     std::string note;

                     CASUAL_CONST_CORRECT_MARSHAL
                     (
                        archive & name;
                        archive & retries;
                        archive & note;
                     )
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
                  };

                  struct Manager
                  {
                     std::vector< Group> groups;

                     CASUAL_CONST_CORRECT_MARSHAL
                     (
                        archive & groups;
                     )
                  };

               } // queue

               namespace service
               {

                  struct Service
                  {
                     Service() = default;
                     Service( std::function< void(Service&)> foreign) { foreign( *this);}

                     std::string name;
                     std::chrono::microseconds timeout = std::chrono::microseconds::zero();
                     std::vector< std::string> routes;

                     CASUAL_CONST_CORRECT_MARSHAL
                     (
                        archive & name;
                        archive & timeout;
                        archive & routes;
                     )
                  };

                  //!
                  //! Trying a new name for 'broker'. If it feels good
                  //! we change the real broker to service::Manager
                  //!
                  //! otherwise, we change this type.
                  //!
                  struct Manager
                  {
                     std::chrono::microseconds default_timeout = std::chrono::microseconds::zero();

                     std::vector< Service> services;

                     CASUAL_CONST_CORRECT_MARSHAL
                     (
                        archive & default_timeout;
                        archive & services;
                     )
                  };


               } // service

               struct Domain
               {
                  std::string name;

                  transaction::Manager transaction;
                  gateway::Manager gateway;
                  queue::Manager queue;
                  service::Manager service;


                  CASUAL_CONST_CORRECT_MARSHAL
                  (
                     archive & name;
                     archive & transaction;
                     archive & gateway;
                     archive & queue;
                     archive & service;
                  )
               };
               static_assert( traits::is_movable< Domain>::value, "not movable");


               struct Request : common::message::basic_request< common::message::Type::domain_configuration_request>
               {

               };

               static_assert( traits::is_movable< Request>::value, "not movable");

               using base_reply = common::message::basic_reply< common::message::Type::domain_configuration_reply>;
               struct Reply : base_reply
               {
                  Domain domain;

                  CASUAL_CONST_CORRECT_MARSHAL(
                     base_reply::marshal( archive);
                     archive & domain;
                  )

               };

               static_assert( traits::is_movable< Reply>::value, "not movable");


               namespace server
               {

                  using base_request = message::basic_request< message::Type::domain_server_configuration_request>;
                  struct Request : base_request
                  {

                  };

                  using base_reply = message::basic_request< message::Type::domain_server_configuration_reply>;
                  struct Reply : base_reply
                  {
                     struct Service
                     {
                        std::string name;
                        std::vector< std::string> routes;

                        CASUAL_CONST_CORRECT_MARSHAL(
                        {
                           archive & name;
                           archive & routes;
                        })
                     };

                     std::vector< std::string> resources;
                     std::vector< std::string> restrictions;
                     std::vector< Service> routes;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_reply::marshal( archive);
                        archive & resources;
                        archive & restrictions;
                        archive & routes;
                     })
                  };

               } // server

            } // configuration



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

                     friend std::ostream& operator << ( std::ostream& out, const Request& value);
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
                  using Registration = basic_request< Type::domain_process_termination_registration>;

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
                  using base_reqeust = message::basic_request< Type::domain_process_lookup_request>;
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

                  using base_reply = basic_reply< Type::domain_process_lookup_reply>;

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

         } // domain

         namespace reverse
         {
            template<>
            struct type_traits< domain::configuration::Request> : detail::type< domain::configuration::Reply> {};

            template<>
            struct type_traits< domain::configuration::server::Request> : detail::type< domain::configuration::server::Reply> {};

            template<>
            struct type_traits< domain::process::lookup::Request> : detail::type< domain::process::lookup::Reply> {};

            template<>
            struct type_traits< domain::process::connect::Request> : detail::type< domain::process::connect::Reply> {};


         } // reverse

      } // message
   } // common

} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MESSAGE_DOMAIN_H_
