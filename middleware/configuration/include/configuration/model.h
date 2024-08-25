//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/environment/variable.h"
#include "common/serialize/macro.h"
#include "common/service/type.h"

#include <vector>
#include <string>
#include <optional>
#include <filesystem>

namespace casual::configuration
{
   namespace model
   {
      namespace system
      {
         namespace resource
         {
            struct Paths
            {
               std::vector< std::string> include;
               std::vector< std::string> library;

               friend auto operator <=> ( const Paths&, const Paths&) = default;

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

            std::vector< std::string> libraries;
            resource::Paths paths;
            std::string note;

            inline friend bool operator == ( const Resource& lhs, const std::string& rhs) { return lhs.key == rhs;}
            friend auto operator <=> ( const Resource&, const Resource&) = default;
            friend bool operator == ( const Resource&, const Resource&) = default;

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
            std::vector< system::Resource> resources;

            Model set_union( Model lhs, Model rhs);
            Model set_difference( Model lhs, Model rhs);
            Model set_intersection( Model lhs, Model rhs);

            friend auto operator <=> ( const Model&, const Model&) = default;

            inline explicit operator bool() const { return ! resources.empty();}

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( resources);
            )
         };
         
      } // system

      namespace domain
      {
         struct Environment
         {
            std::vector< common::environment::Variable> variables;

            Environment set_union( Environment lhs, Environment rhs);
            Environment set_difference( Environment lhs, Environment rhs);
            Environment set_intersection( Environment lhs, Environment rhs);

            friend auto operator <=> ( const Environment&, const Environment&) = default;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( variables);
            )
         };
            
         struct Group
         {
            std::string name;
            bool enabled = true;
            std::vector< std::string> dependencies;
            std::string note;

            inline friend bool operator == ( const Group& lhs, const std::string& rhs) { return lhs.name == rhs;}
            friend auto operator <=> ( const Group&, const Group&) = default;
            friend bool operator == ( const Group&, const Group&) = default;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( name);
               CASUAL_SERIALIZE( enabled);
               CASUAL_SERIALIZE( dependencies);
               CASUAL_SERIALIZE( note);
            )
         };

         struct Lifetime
         {
            bool restart = false;

            friend auto operator <=> ( const Lifetime&, const Lifetime&) = default;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( restart);
            )
         };

         namespace detail
         {
            struct Entity
            {
               std::string alias;
               std::filesystem::path path;
               std::vector< std::string> arguments;

               platform::size::type instances = 1;
               std::vector< std::string> memberships;
               Environment environment;
               Lifetime lifetime;
               bool enabled = true;
               std::string note;

               inline friend bool operator == ( const Entity& lhs, const std::string& rhs) noexcept { return lhs.alias == rhs;}
               friend auto operator <=> ( const Entity&, const Entity&) = default;
               friend bool operator == ( const Entity&, const Entity&) = default;
               
               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( alias);
                  CASUAL_SERIALIZE( path);
                  CASUAL_SERIALIZE( arguments);
                  CASUAL_SERIALIZE( instances);
                  CASUAL_SERIALIZE( memberships);
                  CASUAL_SERIALIZE( environment);
                  CASUAL_SERIALIZE( lifetime);
                  CASUAL_SERIALIZE( enabled);
                  CASUAL_SERIALIZE( note);
               )
            };
         } // detail

         struct Executable : detail::Entity
         {
 
         };

         struct Server : detail::Entity
         {

         };

         struct Model
         {
            std::string name;
            domain::Environment environment;
            std::vector< domain::Group> groups;
            std::vector< domain::Server> servers;
            std::vector< domain::Executable> executables;

            Model set_union( Model lhs, Model rhs);
            Model set_difference( Model lhs, Model rhs);
            Model set_intersection( Model lhs, Model rhs);

            friend auto operator <=> ( const Model&, const Model&) = default;
            
            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( name);
               CASUAL_SERIALIZE( environment);
               CASUAL_SERIALIZE( groups);
               CASUAL_SERIALIZE( servers);
               CASUAL_SERIALIZE( executables);
            )
         };

      } // domain


      namespace service
      {
         namespace restriction
         {
            struct Server
            {
               std::string alias;
               //! a set of regex, to match for allowed services. If empty all advertised services are allowed for the alias
               std::vector< std::string> services;
 
               inline friend bool operator == ( const Server& lhs, const std::string& alias) { return lhs.alias == alias;}
               friend auto operator <=> ( const Server&, const Server&) = default;
               friend bool operator == ( const Server&, const Server&) = default;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( alias);
                  CASUAL_SERIALIZE( services);
               )
            };

         } // restriction
         
         struct Restriction
         {
            std::vector< restriction::Server> servers;

            Restriction set_union( Restriction lhs, Restriction rhs);
            Restriction set_difference( Restriction lhs, Restriction rhs);
            Restriction set_intersection( Restriction lhs, Restriction rhs);

            friend auto operator <=> ( const Restriction&, const Restriction&) = default;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( servers);
            )
         };

         struct Timeout
         {
            using Contract = common::service::execution::timeout::contract::Type;
            
            std::optional< platform::time::unit> duration;
            std::optional< Contract> contract;

            Timeout set_union( Timeout lhs, Timeout rhs);

            friend auto operator <=> ( const Timeout&, const Timeout&) = default;
            inline explicit operator bool() const noexcept { return duration.has_value();}

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( duration);
               CASUAL_SERIALIZE( contract);
            )
         };

         struct Service
         {
            std::string name;
            std::vector< std::string> routes;
            service::Timeout timeout;
            std::optional< common::service::visibility::Type> visibility;
            std::string note;

            inline friend bool operator == ( const Service& lhs, const std::string& name) { return lhs.name == name;}
            friend auto operator <=> ( const Service&, const Service&) = default;
            friend bool operator == ( const Service&, const Service&) = default;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( name);
               CASUAL_SERIALIZE( routes);
               CASUAL_SERIALIZE( timeout);
               CASUAL_SERIALIZE( visibility);
               CASUAL_SERIALIZE( note);
            )
         };

         struct Global
         {
            service::Timeout timeout;
            std::string note;

            Global set_union( Global lhs, Global rhs);

            friend auto operator <=> ( const Global&, const Global&) = default;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( timeout);
               CASUAL_SERIALIZE( note);
            )
         };

         struct Model
         {  
            //! "domain global" settings, for services that are not explicitly configured.
            Global global;

            std::vector< service::Service> services;
            Restriction restriction;

            Model set_union( Model lhs, Model rhs);
            Model set_difference( Model lhs, Model rhs);
            Model set_intersection( Model lhs, Model rhs);

            friend auto operator <=> ( const Model&, const Model&) = default;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( global);
               CASUAL_SERIALIZE( services);
               CASUAL_SERIALIZE( restriction);
            )
         };
      } // service

      namespace transaction
      {
         struct Resource
         {
            std::string name;
            std::string key;
            platform::size::type instances = 1;
            std::string note;

            std::string openinfo;
            std::string closeinfo;

            friend auto operator <=> ( const Resource&, const Resource&) = default;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( name);
               CASUAL_SERIALIZE( key);
               CASUAL_SERIALIZE( instances);
               CASUAL_SERIALIZE( note);
               CASUAL_SERIALIZE( openinfo);
               CASUAL_SERIALIZE( closeinfo);
            )
         };

         //! TODO: maintainence - better name
         struct Mapping 
         {
            std::string alias;
            std::vector< std::string> resources;

            inline friend bool operator == ( const Mapping& lhs, const std::string& alias) { return lhs.alias == alias;} 
            friend auto operator <=> ( const Mapping&, const Mapping&) = default;
            friend bool operator == ( const Mapping&, const Mapping&) = default;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( alias);
               CASUAL_SERIALIZE( resources);
            )
         };

         struct Model
         {
            std::string log;
            std::vector< transaction::Resource> resources;
            std::vector< Mapping> mappings;

            Model set_union( Model lhs, Model rhs);
            Model set_difference( Model lhs, Model rhs);
            Model set_intersection( Model lhs, Model rhs);

            friend auto operator <=> ( const Model&, const Model&) = default;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( log);
               CASUAL_SERIALIZE( resources);
               CASUAL_SERIALIZE( mappings);
            )
         };

      } // transaction

      namespace gateway
      {
         namespace connect
         {
            enum struct Semantic : short
            {
               unknown,
               regular,
               reversed
            };

            inline std::ostream& operator << ( std::ostream& out, Semantic value)
            {
               switch( value)
               {
                  case Semantic::unknown: return out << "unknown";
                  case Semantic::regular: return out << "regular";
                  case Semantic::reversed: return out << "reversed";
               }
               return out << "<unknown>";
            }
         } // connect


         namespace inbound
         {
            namespace connection
            {
               namespace discovery
               {
                  enum struct Directive
                  {
                     localized,
                     forward,
                  };

                  constexpr std::string_view description( Directive value) noexcept
                  {
                     switch( value)
                     {
                        case Directive::forward: return "forward";
                        case Directive::localized: return "localized";
                     }
                     return "<unknown>";
                  }

               } // discovery
               
            } // connection
            
            struct Connection
            {
               std::string address;
               connection::discovery::Directive discovery{};
               std::string note;

               Connection set_union( Connection lhs, Connection rhs);

               friend auto operator <=> ( const Connection&, const Connection&) = default;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( address);
                  CASUAL_SERIALIZE( discovery);
                  CASUAL_SERIALIZE( note);
               )
            };

            struct Limit
            {
               platform::size::type size{};
               platform::size::type messages{};

               friend auto operator <=> ( const Limit&, const Limit&) = default;
               constexpr operator bool() const noexcept { return size > 0 || messages > 0;}

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( size);
                  CASUAL_SERIALIZE( messages);
               )
            };

            struct Group
            {
               connect::Semantic connect = connect::Semantic::unknown;
               std::string alias;
               inbound::Limit limit;
               std::vector< inbound::Connection> connections;
               std::string note;

               inline bool empty() const { return connections.empty();}

               Group set_union( Group lhs, Group rhs);
               Group set_difference( Group lhs, Group rhs);
               Group set_intersection( Group lhs, Group rhs);

               friend auto operator <=> ( const Group&, const Group&) = default;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( connect);
                  CASUAL_SERIALIZE( alias);
                  CASUAL_SERIALIZE( limit);
                  CASUAL_SERIALIZE( connections);
                  CASUAL_SERIALIZE( note);
               )
            };

         } // inbound

         struct Inbound
         {
            std::vector< inbound::Group> groups;

            inline bool empty() const { return groups.empty();}

            Inbound set_union( Inbound lhs, Inbound rhs);
            Inbound set_difference( Inbound lhs, Inbound rhs);
            Inbound set_intersection( Inbound lhs, Inbound rhs);

            friend auto operator <=> ( const Inbound&, const Inbound&) = default;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( groups);
            )
         };

         namespace outbound
         {
            struct Connection
            {
               std::string address;
               std::vector< std::string> services;
               std::vector< std::string> queues;
               std::string note;

               Connection set_union( Connection lhs, Connection rhs);
               Connection set_difference( Connection lhs, Connection rhs);
               Connection set_intersection( Connection lhs, Connection rhs);

               friend auto operator <=> ( const Connection&, const Connection&) = default;

               inline explicit operator bool () const { return ! ( services.empty() && queues.empty());}

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( note);
                  CASUAL_SERIALIZE( address);
                  CASUAL_SERIALIZE( services);
                  CASUAL_SERIALIZE( queues);
               )
            };

            struct Group
            {
               std::string alias;
               connect::Semantic connect = connect::Semantic::unknown;
               std::vector< outbound::Connection> connections;
               platform::size::type order{};
               std::string note;

               Group set_union( Group lhs, Group rhs);
               Group set_difference( Group lhs, Group rhs);
               Group set_intersection( Group lhs, Group rhs);

               inline bool empty() const { return connections.empty();}

               friend auto operator <=> ( const Group&, const Group&) = default;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( alias);
                  CASUAL_SERIALIZE( connect);
                  CASUAL_SERIALIZE( connections);
                  CASUAL_SERIALIZE( order);
                  CASUAL_SERIALIZE( note);
               )
            };

         } // outbound

         struct Outbound
         {
            std::vector< outbound::Group> groups;

            inline bool empty() const { return groups.empty();}

            Outbound set_union( Outbound lhs, Outbound rhs);
            Outbound set_difference( Outbound lhs, Outbound rhs);
            Outbound set_intersection( Outbound lhs, Outbound rhs);

            friend auto operator <=> ( const Outbound&, const Outbound&) = default;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( groups);
            )
         };

         struct Model
         {
            Inbound inbound;
            Outbound outbound;

            Model set_union( Model lhs, Model rhs);
            Model set_difference( Model lhs, Model rhs);
            Model set_intersection( Model lhs, Model rhs);

            friend auto operator <=> ( const Model&, const Model&) = default;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( inbound);
               CASUAL_SERIALIZE( outbound);
            )
         };
         
      } // gateway

      namespace queue
      {
   
         struct Queue
         {
            struct Retry
            {
               platform::size::type count{};
               platform::time::unit delay = platform::time::unit::zero();

               inline auto empty() const { return count == 0 && delay == platform::time::unit::zero();}

               friend auto operator <=> ( const Retry&, const Retry&) = default;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( count);
                  CASUAL_SERIALIZE( delay);
               )
            };

            struct Enable
            {
               bool enqueue = true;
               bool dequeue = true;

               friend auto operator <=> ( const Enable&, const Enable&) = default;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( enqueue);
                  CASUAL_SERIALIZE( dequeue);
               )
            };

            std::string name;
            Retry retry;
            Enable enable;
            std::string note;

            inline friend bool operator == ( const Queue& lhs, const std::string& rhs) { return lhs.name == rhs;}
            friend auto operator <=> ( const Queue&, const Queue&) = default;
            friend bool operator == ( const Queue&, const Queue&) = default;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( name);
               CASUAL_SERIALIZE( retry);
               CASUAL_SERIALIZE( enable);
               CASUAL_SERIALIZE( note);
            )
         };

         struct Group
         {
            std::string alias;
            std::string queuebase;
            std::string note;
            std::vector< Queue> queues;
            std::string directory;
            std::optional< platform::size::type> capacity;

            inline friend bool operator == ( const Group& lhs, const std::string& alias) { return lhs.alias == alias;}
            friend auto operator <=> ( const Group&, const Group&) = default;
            friend bool operator == ( const Group&, const Group&) = default;

            Group set_union( Group lhs, Group rhs);
            Group set_difference( Group lhs, Group rhs);
            Group set_intersection( Group lhs, Group rhs);

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( alias);
               CASUAL_SERIALIZE( queuebase);
               CASUAL_SERIALIZE( note);
               CASUAL_SERIALIZE( queues);
               CASUAL_SERIALIZE( directory);
               CASUAL_SERIALIZE( capacity);
            )
         };

         namespace forward
         {
            struct forward_base
            {
               std::string alias;
               std::string source;
               platform::size::type instances = 1;
               std::string note;
               std::vector< std::string> memberships;

               friend auto operator <=> ( const forward_base&, const forward_base&) = default;

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
                  platform::time::unit delay{};

                  friend auto operator <=> ( const Target&, const Target&) = default;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( queue);
                     CASUAL_SERIALIZE( delay);
                  )
               };

               Target target;

               friend auto operator <=> ( const Queue&, const Queue&) = default;

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

                  friend auto operator <=> ( const Target&, const Target&) = default;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( service);
                  )
               };

               using Reply = Queue::Target;

               Target target;
               std::optional< Reply> reply;

               friend auto operator <=> ( const Service&, const Service&) = default;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  forward_base::serialize( archive);
                  CASUAL_SERIALIZE( target);
                  CASUAL_SERIALIZE( reply);
               )
            };

            struct Group
            {
               std::string alias;
               std::vector< forward::Service> services;
               std::vector< forward::Queue> queues;
               std::string note;
               std::vector< std::string> memberships;

               Group set_union( Group lhs, Group rhs);
               Group set_difference( Group lhs, Group rhs);
               Group set_intersection( Group lhs, Group rhs);

               inline friend bool operator == ( const Group& lhs, const std::string& alias) { return lhs.alias == alias;}
               friend auto operator <=> ( const Group&, const Group&) = default;
               friend bool operator == ( const Group&, const Group&) = default;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( alias);
                  CASUAL_SERIALIZE( services);
                  CASUAL_SERIALIZE( queues);
                  CASUAL_SERIALIZE( note);
                  CASUAL_SERIALIZE( memberships);
               )
            };
         } // forward

         struct Forward
         {
            std::vector< forward::Group> groups;

            Forward set_union( Forward lhs, Forward rhs);
            Forward set_difference( Forward lhs, Forward rhs);
            Forward set_intersection( Forward lhs, Forward rhs);

            friend auto operator <=> ( const Forward&, const Forward&) = default;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( groups);
            )
         };

         struct Model
         {
            std::vector< queue::Group> groups;
            Forward forward;
            std::string note;

            Model set_union( Model lhs, Model rhs);
            Model set_difference( Model lhs, Model rhs);
            Model set_intersection( Model lhs, Model rhs);

            friend auto operator <=> ( const Model&, const Model&) = default;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( groups);
               CASUAL_SERIALIZE( forward);
               CASUAL_SERIALIZE( note);
            )
         };

      } // queue

   } // model

   struct Model
   {
      model::system::Model system;
      model::domain::Model domain;
      model::transaction::Model transaction;
      model::service::Model service;
      model::queue::Model queue;
      model::gateway::Model gateway;

      inline friend Model operator + ( Model lhs, Model rhs) { return set_union( std::move( lhs), std::move( rhs));}

      friend Model normalize( Model model);
      friend Model set_union( Model lhs, Model rhs);
      friend Model set_difference( Model lhs, Model rhs);
      friend Model set_intersection( Model lhs, Model rhs);

      friend auto operator <=> ( const Model&, const Model&) = default;

      CASUAL_CONST_CORRECT_SERIALIZE(
         CASUAL_SERIALIZE( system);
         CASUAL_SERIALIZE( domain);
         CASUAL_SERIALIZE( transaction);
         CASUAL_SERIALIZE( service);
         CASUAL_SERIALIZE( queue);
         CASUAL_SERIALIZE( gateway);
      )
   };



} // casual::configuration