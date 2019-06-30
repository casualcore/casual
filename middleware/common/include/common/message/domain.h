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

                     CASUAL_CONST_CORRECT_SERIALIZE(
                        CASUAL_SERIALIZE( name);
                        CASUAL_SERIALIZE( key);
                        CASUAL_SERIALIZE( instances);
                        CASUAL_SERIALIZE( note);
                        CASUAL_SERIALIZE( openinfo);
                        CASUAL_SERIALIZE( closeinfo);
                     )
                  };

                  struct Manager
                  {
                     std::string log;
                     std::vector< Resource> resources;

                     CASUAL_CONST_CORRECT_SERIALIZE
                     (
                        CASUAL_SERIALIZE( log);
                        CASUAL_SERIALIZE( resources);
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

                        CASUAL_CONST_CORRECT_SERIALIZE
                        (
                           CASUAL_SERIALIZE( size);
                           CASUAL_SERIALIZE( messages);
                        )
                     };

                     std::string address;
                     Limit limit;

                     CASUAL_CONST_CORRECT_SERIALIZE
                     (
                        CASUAL_SERIALIZE( address);
                        CASUAL_SERIALIZE( limit);
                     )
                  };

                  struct Connection
                  {
                     bool restart = true;
                     std::string address;
                     std::string note;
                     std::vector< std::string> services;
                     std::vector< std::string> queues;


                     CASUAL_CONST_CORRECT_SERIALIZE
                     (
                        CASUAL_SERIALIZE( restart);
                        CASUAL_SERIALIZE( address);
                        CASUAL_SERIALIZE( note);
                        CASUAL_SERIALIZE( services);
                        CASUAL_SERIALIZE( queues);
                     )
                  };

                  struct Manager
                  {
                     std::vector< Listener> listeners;
                     std::vector< Connection> connections;

                     CASUAL_CONST_CORRECT_SERIALIZE
                     (
                        CASUAL_SERIALIZE( listeners);
                        CASUAL_SERIALIZE( connections);
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

                     CASUAL_CONST_CORRECT_SERIALIZE
                     (
                        CASUAL_SERIALIZE( name);
                        CASUAL_SERIALIZE( retries);
                        CASUAL_SERIALIZE( note);
                     )
                  };

                  struct Group
                  {
                     std::string name;
                     std::string queuebase;
                     std::string note;
                     std::vector< Queue> queues;

                     CASUAL_CONST_CORRECT_SERIALIZE
                     (
                        CASUAL_SERIALIZE( name);
                        CASUAL_SERIALIZE( queuebase);
                        CASUAL_SERIALIZE( note);
                        CASUAL_SERIALIZE( queues);
                     )
                  };

                  struct Manager
                  {
                     std::vector< Group> groups;

                     CASUAL_CONST_CORRECT_SERIALIZE
                     (
                        CASUAL_SERIALIZE( groups);
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

                     CASUAL_CONST_CORRECT_SERIALIZE
                     (
                        CASUAL_SERIALIZE( name);
                        CASUAL_SERIALIZE( timeout);
                        CASUAL_SERIALIZE( routes);
                     )
                  };


                  struct Manager
                  {
                     common::platform::time::unit default_timeout = common::platform::time::unit::zero();

                     std::vector< Service> services;

                     CASUAL_CONST_CORRECT_SERIALIZE
                     (
                        CASUAL_SERIALIZE( default_timeout);
                        CASUAL_SERIALIZE( services);
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


                  CASUAL_CONST_CORRECT_SERIALIZE
                  (
                     CASUAL_SERIALIZE( name);
                     CASUAL_SERIALIZE( transaction);
                     CASUAL_SERIALIZE( gateway);
                     CASUAL_SERIALIZE( queue);
                     CASUAL_SERIALIZE( service);
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

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_reply::serialize( archive);
                     CASUAL_SERIALIZE( domain);
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
                        struct Route
                        {
                           std::string name;
                           std::vector< std::string> routes;

                           CASUAL_CONST_CORRECT_SERIALIZE(
                           {
                              CASUAL_SERIALIZE( name);
                              CASUAL_SERIALIZE( routes);
                           })
                        };

                        std::vector< std::string> restrictions;
                        std::vector< Route> routes;

                        CASUAL_CONST_CORRECT_SERIALIZE(
                        {
                           CASUAL_SERIALIZE( restrictions);
                           CASUAL_SERIALIZE( routes);
                        })
                     };

                     std::vector< std::string> resources;
                     Service service;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_reply::serialize( archive);
                        CASUAL_SERIALIZE( resources);
                        CASUAL_SERIALIZE( service);
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

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_type::serialize( archive);
                        CASUAL_SERIALIZE( process);
                        CASUAL_SERIALIZE( identification);
                     })
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

                     inline friend std::ostream& operator << ( std::ostream& out, Directive value)
                     {
                        switch( value)
                        {
                           case Directive::start: return out << "start";
                           case Directive::singleton: return out << "singleton";
                           case Directive::shutdown: return out << "shutdown";
                           default: return out << "unknown";
                        }
                     }

                     Directive directive = Directive::start;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_type::serialize( archive);
                        CASUAL_SERIALIZE( directive);
                     })
                  };
                  static_assert( traits::is_movable< Reply>::value, "not movable");

               } // connect




               namespace lookup
               {
                  using base_request = message::basic_request< Type::domain_process_lookup_request>;
                  struct Request : base_request
                  {
                     enum class Directive : short
                     {
                        wait,
                        direct
                     };

                     inline friend std::ostream& operator << ( std::ostream& out, Directive value)
                     {
                        switch( value)
                        {
                           case Directive::wait: return out << "wait";
                           case Directive::direct: return out << "direct";
                           default: return out << "unknown";
                        }
                     }

                     Uuid identification;
                     strong::process::id pid;
                     Directive directive = Directive::wait;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_request::serialize( archive);
                        CASUAL_SERIALIZE( identification);
                        CASUAL_SERIALIZE( pid);
                        CASUAL_SERIALIZE( directive);
                     })

                  };
                  static_assert( traits::is_movable< Request>::value, "not movable");

                  using base_reply = basic_reply< Type::domain_process_lookup_reply>;

                  struct Reply : base_reply
                  {
                     Uuid identification;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_reply::serialize( archive);
                        CASUAL_SERIALIZE( identification);
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

                        CASUAL_CONST_CORRECT_SERIALIZE(
                        {
                           base_request::serialize( archive);
                           CASUAL_SERIALIZE( processes);
                        })
                     };

                     using base_reply = basic_request< Type::domain_process_prepare_shutdown_reply>;
                     struct Reply : base_reply
                     {
                        using base_reply::base_reply;

                        std::vector< common::process::Handle> processes;

                        CASUAL_CONST_CORRECT_SERIALIZE(
                        {
                           base_reply::serialize( archive);
                           CASUAL_SERIALIZE( processes);
                        })
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


