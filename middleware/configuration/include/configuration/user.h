//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once



#include "common/serialize/macro.h"
#include "casual/platform.h"

#include <string>
#include <vector>
#include <optional>
#include <filesystem>

namespace casual
{
   namespace configuration::user
   {

      namespace system
      {
         namespace resource
         {
            struct Paths
            {
               std::optional< std::vector< std::string>> include;
               std::optional< std::vector< std::string>> library;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( include);
                  CASUAL_SERIALIZE( library);
               )
            };
            
         } // resource

         struct Resource 
         {
            std::string key;
            std::string server;
            std::string xa_struct_name;

            std::optional< std::string> note;

            std::optional< std::vector< std::string>> libraries;
            std::optional< resource::Paths> paths;


            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( key);
               CASUAL_SERIALIZE( server);
               CASUAL_SERIALIZE( xa_struct_name);
               CASUAL_SERIALIZE( libraries);
               CASUAL_SERIALIZE( paths);
               CASUAL_SERIALIZE( note);
            )
         };

         struct Model 
         {
            std::optional< std::vector< system::Resource>> resources;

            friend Model normalize( Model model);
            
            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( resources);
            )
         };
         
      } // system

      namespace domain
      {
         namespace environment
         {
            struct Variable
            {
               std::string key;
               std::string value;

               friend inline bool operator == ( const Variable& l, const Variable& r) { return l.key == r.key && l.value == r.value;}

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( key);
                  CASUAL_SERIALIZE( value);
               )
            };

         } // environment

         struct Environment
         {
            std::optional< std::vector< std::filesystem::path>> files;
            std::optional< std::vector< environment::Variable>> variables;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( files);
               CASUAL_SERIALIZE( variables);
            )
         };

         struct Group
         {
            std::string name;
            std::optional< std::string> note;
            std::optional< bool> enabled;

            std::optional< std::vector< std::string>> resources;
            std::optional< std::vector< std::string>> dependencies;

            friend inline bool operator == ( const Group& l, const std::string& r) { return l.name == r;}

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( name);
               CASUAL_SERIALIZE( note);
               CASUAL_SERIALIZE( enabled);
               CASUAL_SERIALIZE( resources);
               CASUAL_SERIALIZE( dependencies);
            )
         };

         namespace executable
         {
            struct Default
            {
               platform::size::type instances = 1;
               std::optional< std::vector< std::string>> memberships;
               std::optional< Environment> environment;
               bool restart = false;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( instances);
                  CASUAL_SERIALIZE( restart);
                  CASUAL_SERIALIZE( memberships);
                  CASUAL_SERIALIZE( environment);
               )
            };

         } // executable

         struct Executable
         {
            std::filesystem::path path;
            std::optional< std::string> alias;
            std::optional< std::string> note;

            std::optional< std::vector< std::string>> arguments;

            std::optional< platform::size::type> instances;
            std::optional< std::vector< std::string>> memberships;
            std::optional< Environment> environment;
            std::optional< bool> restart;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( path);
               CASUAL_SERIALIZE( alias);
               CASUAL_SERIALIZE( note);
               CASUAL_SERIALIZE( arguments);
               CASUAL_SERIALIZE( instances);
               CASUAL_SERIALIZE( memberships);
               CASUAL_SERIALIZE( environment);
               CASUAL_SERIALIZE( restart);
               
            )
         };

         struct Server : Executable
         {
            std::optional< std::vector< std::string>> restrictions;
            std::optional< std::vector< std::string>> resources;

            CASUAL_CONST_CORRECT_SERIALIZE(
               Executable::serialize( archive);
               CASUAL_SERIALIZE( restrictions);
               CASUAL_SERIALIZE( resources);
            )
         };

         namespace service
         {
            namespace execution
            {
               struct Timeout
               {
                  std::optional< std::string> duration;
                  std::optional< std::string> contract;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( duration);
                     CASUAL_SERIALIZE( contract);
                  )
               };
            } // execution

            struct Execution
            {
               std::optional< execution::Timeout> timeout;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( timeout);
               )
            };

            //! Represent the default settings for services, within a
            //! 'configuration file' 
            struct Default
            {
               std::optional< service::Execution> execution;
               std::optional< std::string> visibility;

               //! @deprecated
               std::optional< std::string> timeout;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( execution);
                  CASUAL_SERIALIZE( visibility);

                  CASUAL_SERIALIZE( timeout);
               )
            };

         } // service

         struct Service
         {
            std::string name;
            std::optional< std::string> note;
            std::optional< service::Execution> execution;
            std::optional< std::vector< std::string>> routes;
            std::optional< std::string> visibility;
            
            //! @deprecated
            std::optional< std::string> timeout;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( name);
               CASUAL_SERIALIZE( note);
               CASUAL_SERIALIZE( execution);
               CASUAL_SERIALIZE( routes);
               CASUAL_SERIALIZE( visibility);

               CASUAL_SERIALIZE( timeout);
            )
         };

         namespace global
         {
            //! Represent the "domain global" default settings for all services
            //! that does not "override" with specific settings
            struct Service
            {
               std::optional< std::string> note;
               std::optional< domain::service::Execution> execution;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( note);
                  CASUAL_SERIALIZE( execution);
               )
            };
            
         } // global

         namespace transaction
         {
            namespace resource
            {
               struct Default
               {
                  std::optional< std::string> key;
                  std::optional< platform::size::type> instances = 1;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( key);
                     CASUAL_SERIALIZE( instances);
                  )
               };
            } // resource

            struct Resource
            {
               std::string name;
               std::optional< std::string> key;
               std::optional< platform::size::type> instances;
               
               std::optional< std::string> note;

               std::optional< std::string> openinfo;
               std::optional< std::string> closeinfo;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( name);
                  CASUAL_SERIALIZE( key);
                  CASUAL_SERIALIZE( instances);
                  CASUAL_SERIALIZE( note);
                  CASUAL_SERIALIZE( openinfo);
                  CASUAL_SERIALIZE( closeinfo);
               )               
            };

            namespace manager
            {
               struct Default
               {
                  resource::Default resource;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( resource);
                  )
               };

            } // manager

            struct Manager
            {
               std::optional< manager::Default> defaults;

               std::string log;
               std::vector< Resource> resources;

               friend Manager normalize( Manager manager);

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE_NAME( defaults, "default");
                  CASUAL_SERIALIZE( log);
                  CASUAL_SERIALIZE( resources);
               )
            };


         } // transaction

         namespace gateway
         {
            namespace inbound
            {
               struct Limit
               {
                  std::optional< platform::size::type> size;
                  std::optional< platform::size::type> messages;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( size);
                     CASUAL_SERIALIZE( messages);
                  )
               };

               namespace connection
               {
                  struct Discovery
                  {
                     bool forward = false;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                        CASUAL_SERIALIZE( forward);
                     )
                  };
                  
               } // connection

               struct Connection
               {
                  std::string address;
                  std::optional< connection::Discovery> discovery;
                  std::optional< std::string> note;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( address);
                     CASUAL_SERIALIZE( discovery);
                     CASUAL_SERIALIZE( note);
                  )
               };

               struct Default
               {
                  struct Connection
                  {
                     std::optional< connection::Discovery> discovery;
                     
                     CASUAL_CONST_CORRECT_SERIALIZE(
                        CASUAL_SERIALIZE( discovery);
                     )
                  };

                  std::optional< inbound::Limit> limit;
                  std::optional< Connection> connection;
                  std::optional< std::string> note;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( note);
                     CASUAL_SERIALIZE( limit);
                     CASUAL_SERIALIZE( connection);
                  )
               };

               struct Group
               {
                  std::optional< std::string> alias;
                  std::optional< inbound::Limit> limit;
                  std::vector< inbound::Connection> connections;
                  std::optional< std::string> note;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( alias);
                     CASUAL_SERIALIZE( note);
                     CASUAL_SERIALIZE( limit);
                     CASUAL_SERIALIZE( connections);
                  )
               };
            } // inbound

            struct Inbound
            {
               std::optional< inbound::Default> defaults;
               std::vector< inbound::Group> groups;

               friend Inbound normalize( Inbound inbound);

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE_NAME( defaults, "default");
                  CASUAL_SERIALIZE( groups);
               )
            };

            namespace outbound
            {
               struct Connection
               {
                  std::string address;
                  std::optional< std::vector< std::string>> services;
                  std::optional< std::vector< std::string>> queues;
                  std::optional< std::string> note;


                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( address);
                     CASUAL_SERIALIZE( note);
                     CASUAL_SERIALIZE( services);
                     CASUAL_SERIALIZE( queues);
                  ) 
               };

               struct Group
               {
                  std::optional< std::string> alias;
                  std::optional< std::string> note;
                  std::optional< platform::size::type> order;
                  std::vector< outbound::Connection> connections;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( alias);
                     CASUAL_SERIALIZE( note);
                     CASUAL_SERIALIZE( order);
                     CASUAL_SERIALIZE( connections);
                  )
               };
            }

            struct Outbound
            {
               std::vector< outbound::Group> groups;

               friend Outbound normalize( Outbound outbound);

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( groups);
               )
            };


            struct Reverse
            {
               std::optional< gateway::Inbound> inbound;
               std::optional< gateway::Outbound> outbound;


               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( inbound);
                  CASUAL_SERIALIZE( outbound);
               )
            };


               //! @deprecated
            namespace listener
            {
               struct Default
               {
                  inbound::Limit limit;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( limit);
                  )
               };

            } // listener

            //! @deprecated
            struct Listener
            {
               std::optional< std::string> note;
               std::string address;
               std::optional< inbound::Limit> limit;
               
               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( note);
                  CASUAL_SERIALIZE( address);
                  CASUAL_SERIALIZE( limit);
               )
            };

            namespace connection
            {
               //! @deprecated
               struct Default
               {
                  bool restart = true;

                  //! @deprecated Makes "no sense" to have a default address. 
                  std::string address;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( restart);
                     CASUAL_SERIALIZE( address);
                  )
               };
            } // connection

            //! @deprecated
            struct Connection
            {
               std::optional< std::string> note;
               std::string address;
               std::optional< std::vector< std::string>> services;
               std::optional< std::vector< std::string>> queues;
               std::optional< bool> restart;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( note);
                  CASUAL_SERIALIZE( address);
                  CASUAL_SERIALIZE( services);
                  CASUAL_SERIALIZE( queues);
                  CASUAL_SERIALIZE( restart);
               )               
            };

            namespace manager
            {
               struct Default
               {
                  //! @deprecated
                  //! @{
                  std::optional< listener::Default> listener;
                  std::optional< connection::Default> connection;
                  //! @}

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( listener);
                     CASUAL_SERIALIZE( connection);
                  )
               };
            } // manager


            struct Manager
            {
               std::optional< manager::Default> defaults;

               std::optional<gateway::Inbound> inbound;
               std::optional< gateway::Outbound> outbound;

               std::optional< Reverse> reverse;

               //! @deprecated
               //! @{
               std::optional< std::vector< gateway::Listener>> listeners;
               std::optional< std::vector< gateway::Connection>> connections;
               //! @}

               friend Manager normalize( Manager manager);

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE_NAME( defaults, "default");
                  CASUAL_SERIALIZE( inbound);
                  CASUAL_SERIALIZE( outbound);
                  CASUAL_SERIALIZE( reverse);

                  //! @deprecated
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
                  std::optional< platform::size::type> count;
                  std::optional< std::string> delay;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( count);
                     CASUAL_SERIALIZE( delay);
                  )
               };

               struct Enable
               {
                  bool enqueue = true;
                  bool dequeue = true;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( enqueue);
                     CASUAL_SERIALIZE( dequeue);
                  )
               };

               struct Default
               {
                  std::optional< Retry> retry;
               
                  //! @deprecated
                  std::optional< platform::size::type> retries;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( retry);
                     CASUAL_SERIALIZE( retries);
                  )
               };

               std::string name;
               std::optional< Retry> retry;
               std::optional< Enable> enable;
               std::optional< std::string> note;

               //! @deprecated
               std::optional< platform::size::type> retries;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( name);
                  CASUAL_SERIALIZE( retry);
                  CASUAL_SERIALIZE( enable);
                  CASUAL_SERIALIZE( note);

                  CASUAL_SERIALIZE( retries);
               )
            };

            struct Group
            {
               std::optional< std::string> alias;
               std::optional< std::string> queuebase;
               std::optional< std::string> note;
               std::optional< std::string> capacity;
               std::vector< Queue> queues;

               //! @deprecated
               std::optional< std::string> name;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( alias);
                  CASUAL_SERIALIZE( queuebase);
                  CASUAL_SERIALIZE( note);
                  CASUAL_SERIALIZE( capacity);
                  CASUAL_SERIALIZE( queues);
                  CASUAL_SERIALIZE( name);
               )
            };

            namespace forward
            {
               struct Default
               {
                  struct Service
                  {
                     struct Reply
                     {
                        std::string delay;

                        CASUAL_CONST_CORRECT_SERIALIZE(
                           CASUAL_SERIALIZE( delay);
                        )
                     };

                     std::optional< platform::size::type> instances;
                     std::optional< Reply> reply;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                        CASUAL_SERIALIZE( instances);
                        CASUAL_SERIALIZE( reply);
                     )

                  };

                  struct Queue
                  {
                     struct Target
                     {
                        std::string delay;

                        CASUAL_CONST_CORRECT_SERIALIZE(
                           CASUAL_SERIALIZE( delay);
                        )
                     };

                     std::optional< platform::size::type> instances;
                     std::optional< Target> target;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                        CASUAL_SERIALIZE( instances);
                        CASUAL_SERIALIZE( target);
                     )
                  };

                  std::optional< Service> service;
                  std::optional< Queue> queue;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( service);
                     CASUAL_SERIALIZE( queue);
                  )
               };

               struct forward_base
               {
                  std::optional< std::string> alias;
                  std::string source;
                  std::optional< platform::size::type> instances;
                  std::optional< std::string> note;
                  std::optional< std::vector< std::string>> memberships;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( alias);
                     CASUAL_SERIALIZE( source);
                     CASUAL_SERIALIZE( instances);
                     CASUAL_SERIALIZE( note);
                     CASUAL_SERIALIZE( memberships);
                  )
               };

               struct Queue : forward_base
               {
                  struct Target
                  {
                     std::string queue;
                     std::optional< std::string> delay;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                        CASUAL_SERIALIZE( queue);
                        CASUAL_SERIALIZE( delay);
                     )
                  };

                  Target target;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     forward_base::serialize( archive);
                     CASUAL_SERIALIZE( target);
                  )
               };

               struct Service : forward_base
               {
                  struct Target
                  {
                     std::string service;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                        CASUAL_SERIALIZE( service);
                     )
                  };

                  using Reply = Queue::Target;

                  Target target;
                  std::optional< Reply> reply;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     forward_base::serialize( archive);
                     CASUAL_SERIALIZE( target);
                     CASUAL_SERIALIZE( reply);
                  )
               };

               struct Group
               {
                  std::optional< std::string> alias;
                  std::optional< std::vector< forward::Service>> services;
                  std::optional< std::vector< forward::Queue>> queues;
                  std::optional< std::string> note;
                  std::optional< std::vector< std::string>> memberships;


                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( alias);
                     CASUAL_SERIALIZE( note);
                     CASUAL_SERIALIZE( services);
                     CASUAL_SERIALIZE( queues);
                     CASUAL_SERIALIZE( memberships);
                  )
               };
            } // forward

            struct Forward
            {
               std::optional< forward::Default> defaults;
               std::optional< std::vector< forward::Group>> groups;

               friend Forward normalize( Forward forward);

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE_NAME( defaults, "default");
                  CASUAL_SERIALIZE( groups);
               )
            };

            struct Manager
            {
               struct Default
               {
                  std::optional< Queue::Default> queue;
                  std::optional< std::string> directory;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( queue);
                     CASUAL_SERIALIZE( directory);
                  )
               };

               std::optional< Default> defaults;
               std::optional< std::vector< Group>> groups;
               std::optional< Forward> forward;

               std::optional< std::string> note;

               friend Manager normalize( Manager manager);

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( note);
                  CASUAL_SERIALIZE_NAME( defaults, "default");
                  CASUAL_SERIALIZE( groups);
                  CASUAL_SERIALIZE( forward);
               )
            };

         } // queue


         //! Default settings within a configuration file. This is only to help
         //! user to have default settings for whats defined within one file. Does not
         //! effect other "accumulated" files.
         struct Default
         {
            std::optional< std::string> note;

            std::optional< executable::Default> server;
            std::optional< executable::Default> executable;

            //! Represent the default settings for the set of defined `services`.
            //! Think of it as the initial value of a service that is (possible in part) 
            //! overridden by the explicit settings for a given defined service.
            std::optional< service::Default> service;

            //! @deprecated
            std::optional< Environment> environment;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( note);
               CASUAL_SERIALIZE( server);
               CASUAL_SERIALIZE( executable);
               CASUAL_SERIALIZE( service);
               CASUAL_SERIALIZE( environment);
            )
         };

         //! Represent "domain global" configuration. 
         struct Global
         {
            std::optional< std::string> note;
            std::optional< domain::global::Service> service;
            
            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( note);
               CASUAL_SERIALIZE( service);
            )
         };

         struct Model
         {
            std::optional< std::string> name;
            std::optional< std::string> note;
            std::optional< domain::Global> global;
            std::optional< domain::Default> defaults;

            std::optional< domain::Environment> environment;

            std::optional< domain::transaction::Manager> transaction;

            std::optional< std::vector< domain::Group>> groups;
            std::optional< std::vector< domain::Server>> servers;
            std::optional< std::vector< domain::Executable>> executables;
            
            std::optional< std::vector< domain::Service>> services;

            std::optional< domain::gateway::Manager> gateway;
            std::optional< domain::queue::Manager> queue;

            //! @deprecated
            std::optional< domain::global::Service> service;

            friend Model normalize( Model domain);

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( name);
               CASUAL_SERIALIZE( note);
               CASUAL_SERIALIZE( global);
               CASUAL_SERIALIZE_NAME( defaults, "default");
               CASUAL_SERIALIZE( environment);
               CASUAL_SERIALIZE( transaction);
               CASUAL_SERIALIZE( groups);
               CASUAL_SERIALIZE( servers);
               CASUAL_SERIALIZE( executables);
               CASUAL_SERIALIZE( services);
               CASUAL_SERIALIZE( gateway);
               CASUAL_SERIALIZE( queue);
               CASUAL_SERIALIZE( service);
            )
         };

      } // domain


      struct Model
      {
         std::optional< system::Model> system;
         std::optional< domain::Model> domain;

         //! Normalizes the 'model'. The most important work it
         //! does is to handle deprecated stuff:
         //!  * warn (log) user and suggest non deprecated alternatives.
         //!  * transform/move from deprecated part of the model to 
         //!    non deprecated alternatives.
         //!
         //! This keep the knowledge and complexity of "moving the user
         //! configuration forward in a backward compatibility manner" to
         //! this user model, and the transformation to the _internal_ model
         //! does not need to bother with deprecated stuff (it's somewhat a 
         //! a pain to manage, with all the optionals...)
         friend Model normalize( Model model);


         //! @deprecated use system
         std::optional< std::vector< system::Resource>> resources;

         CASUAL_CONST_CORRECT_SERIALIZE(
            CASUAL_SERIALIZE( system);
            CASUAL_SERIALIZE( domain);

            CASUAL_SERIALIZE( resources);
         )

      };

   } // configuration::user
} // casual


