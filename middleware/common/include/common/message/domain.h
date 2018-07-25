//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


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
                     platform::size::type instances = 0;
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
                     struct Limit
                     {
                        platform::size::type size = 0;
                        platform::size::type messages = 0;

                        CASUAL_CONST_CORRECT_MARSHAL
                        (
                           archive & size;
                           archive & messages;
                        )
                     };

                     std::string address;
                     Limit limit;

                     CASUAL_CONST_CORRECT_MARSHAL
                     (
                        archive & address;
                        archive & limit;
                     )
                  };

                  struct Connection
                  {
                     bool restart = true;
                     std::string address;
                     std::string note;
                     std::vector< std::string> services;
                     std::vector< std::string> queues;


                     CASUAL_CONST_CORRECT_MARSHAL
                     (
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
                     platform::size::type retries = 0;
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
                     common::platform::time::unit timeout = common::platform::time::unit::zero();
                     std::vector< std::string> routes;

                     CASUAL_CONST_CORRECT_MARSHAL
                     (
                        archive & name;
                        archive & timeout;
                        archive & routes;
                     )
                  };


                  struct Manager
                  {
                     common::platform::time::unit default_timeout = common::platform::time::unit::zero();

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
                     enum class Directive : short
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

                     friend std::ostream& operator << ( std::ostream& out, const Directive& value);
                     friend std::ostream& operator << ( std::ostream& out, const Reply& value);
                  };
                  static_assert( traits::is_movable< Reply>::value, "not movable");

               } // connect




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
                     strong::process::id pid;
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

               namespace prepare
               {
                  namespace shutdown
                  {
                     using base_request = basic_request< Type::domain_process_prepare_shutdown_request>;
                     struct Request : base_request
                     {
                        using base_request::base_request;

                        std::vector< common::process::Handle> processes;

                        CASUAL_CONST_CORRECT_MARSHAL(
                        {
                           base_request::marshal( archive);
                           archive & processes;
                        })

                        friend std::ostream& operator << ( std::ostream& out, const Request& value);
                     };

                     using base_reply = basic_request< Type::domain_process_prepare_shutdown_reply>;
                     struct Reply : base_reply
                     {
                        using base_reply::base_reply;

                        std::vector< common::process::Handle> processes;

                        CASUAL_CONST_CORRECT_MARSHAL(
                        {
                           base_reply::marshal( archive);
                           archive & processes;
                        })

                        friend std::ostream& operator << ( std::ostream& out, const Reply& value);
                     };


                  } // shutdown
               } // prepare

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


            template<>
            struct type_traits< domain::process::prepare::shutdown::Request> : detail::type< domain::process::prepare::shutdown::Reply> {};


         } // reverse

      } // message
   } // common

} // casual


