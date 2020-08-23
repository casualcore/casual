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
                     struct Retry 
                     {
                        platform::size::type count = 0;
                        platform::time::unit delay{};

                        CASUAL_CONST_CORRECT_SERIALIZE
                        (
                           CASUAL_SERIALIZE( count);
                           CASUAL_SERIALIZE( delay);
                        )
                     };
                     std::string name;
                     Retry retry;
                     std::string note;

                     CASUAL_CONST_CORRECT_SERIALIZE
                     (
                        CASUAL_SERIALIZE( name);
                        CASUAL_SERIALIZE( retry);
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
                     platform::time::unit timeout = platform::time::unit::zero();
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
                     platform::time::unit default_timeout = platform::time::unit::zero();

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

               using base_request = common::message::basic_request< common::message::Type::domain_configuration_request>;
               struct Request : base_request
               {
                  using base_request::base_request;
               };

               using base_reply = common::message::basic_reply< common::message::Type::domain_configuration_reply>;
               struct Reply : base_reply
               {
                  Domain domain;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_reply::serialize( archive);
                     CASUAL_SERIALIZE( domain);
                  )

               };

               namespace server
               {
                  using base_request = message::basic_request< message::Type::domain_server_configuration_request>;
                  struct Request : base_request
                  {
                     using base_request::base_request;
                  };

                  using base_reply = message::basic_request< message::Type::domain_server_configuration_reply>;
                  struct Reply : base_reply
                  {
                     std::vector< std::string> resources;
                     std::vector< std::string> restrictions;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_reply::serialize( archive);
                        CASUAL_SERIALIZE( resources);
                        CASUAL_SERIALIZE( restrictions);
                     })
                  };

               } // server
            } // configuration

            namespace process
            {
               namespace connect
               {

                  template< message::Type type>
                  struct basic_request : message::basic_request< type>
                  {
                     using base_type = message::basic_request< type>;;
                     using base_type::base_type;

                     Uuid identification;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_type::serialize( archive);
                        CASUAL_SERIALIZE( identification);
                     })
                  };

                  using Request = basic_request< Type::domain_process_connect_request>;

                  template< message::Type type>
                  struct basic_reply : message::basic_message< type>
                  {
                     using base_type =  message::basic_message< type>;
                     using base_type::base_type;

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
                        }
                        return out << "unknown";
                     }

                     Directive directive = Directive::start;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_type::serialize( archive);
                        CASUAL_SERIALIZE( directive);
                     })
                  };

                  using Reply = basic_reply< Type::domain_process_connect_reply>;

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
                        }
                        return out << "unknown";
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

                     using base_reply = basic_reply< Type::domain_process_prepare_shutdown_reply>;
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

            namespace instance
            {
               namespace global
               {
                  namespace state
                  {
                     using Request = basic_request< Type::domain_instance_global_state_request>;
                     
                     using base_reply = basic_message< Type::domain_instance_global_state_reply>;
                     struct Reply : base_reply
                     {
                        using base_reply::base_reply;

                        struct 
                        {
                           std::vector< environment::Variable> variables;
                           
                           CASUAL_CONST_CORRECT_SERIALIZE(
                           {
                              CASUAL_SERIALIZE( variables);
                           })
                           
                        } environment;

                        struct 
                        {
                           std::string alias;
                           platform::size::type index{};

                           CASUAL_CONST_CORRECT_SERIALIZE(
                           {
                              CASUAL_SERIALIZE( alias);
                              CASUAL_SERIALIZE( index);
                           })

                        } instance;

                        struct
                        {
                           common::process::Handle handle;
                           std::string path;

                           CASUAL_CONST_CORRECT_SERIALIZE(
                           {
                              CASUAL_SERIALIZE( handle);
                              CASUAL_SERIALIZE( path);
                           })

                        } process;

                        CASUAL_CONST_CORRECT_SERIALIZE(
                        {
                           base_reply::serialize( archive);
                           CASUAL_SERIALIZE( environment);
                           CASUAL_SERIALIZE( instance);
                           CASUAL_SERIALIZE( process);
                        })
                     };
                     
                  } // state
               } // global
            } // instance
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

            template<>
            struct type_traits< domain::instance::global::state::Request> : detail::type< domain::instance::global::state::Reply> {};

         } // reverse

      } // message
   } // common

} // casual


