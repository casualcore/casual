//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/environment/variable.h"
#include "common/compare.h"
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
            struct Paths : common::Compare< Paths>
            {
               std::vector< std::string> include;
               std::vector< std::string> library;

               inline auto tie() const { return std::tie( include, library);}

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( include);
                  CASUAL_SERIALIZE( library);
               )
            };
            
         } // resource

         struct Resource : common::Compare< Resource>
         {
            std::string key;
            std::string server;
            std::string xa_struct_name;

            std::vector< std::string> libraries;
            resource::Paths paths;
            std::string note;

            inline friend bool operator == ( const Resource& lhs, const std::string& rhs) { return lhs.key == rhs;}

            inline auto tie() const { return std::tie( key, server, xa_struct_name, libraries, paths, note);}

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( key);
               CASUAL_SERIALIZE( server);
               CASUAL_SERIALIZE( xa_struct_name);
               CASUAL_SERIALIZE( libraries);
               CASUAL_SERIALIZE( paths);
               CASUAL_SERIALIZE( note);
            )
         };

         struct Model : common::Compare< Model>
         {
            std::vector< system::Resource> resources;

            Model set_union( Model lhs, Model rhs);
            Model set_difference( Model lhs, Model rhs);
            Model set_intersection( Model lhs, Model rhs);

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( resources);
            )

            inline auto tie() const { return std::tie( resources);}
         };
         
      } // system

      namespace domain
      {
         struct Environment : common::Compare< Environment>
         {
            std::vector< common::environment::Variable> variables;

            Environment set_union( Environment lhs, Environment rhs);
            Environment set_difference( Environment lhs, Environment rhs);
            Environment set_intersection( Environment lhs, Environment rhs);

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( variables);
            )

            inline auto tie() const { return std::tie( variables);}
         };
            
         struct Group : common::Compare< Group>
         {
            std::string name;
            std::string note;

            std::vector< std::string> dependencies;

            inline friend bool operator == ( const Group& lhs, const std::string& rhs) { return lhs.name == rhs;}

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( name);
               CASUAL_SERIALIZE( note);
               CASUAL_SERIALIZE( dependencies);
            )

            inline auto tie() const { return std::tie( name, note, dependencies);}
         };

         struct Lifetime : common::Compare< Lifetime>
         {
            bool restart = false;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( restart);
            )

            inline auto tie() const { return std::tie( restart);}
         };

         namespace detail
         {
            struct Entity : common::Compare< Entity>
            {
               std::string alias;
               std::filesystem::path path;
               std::vector< std::string> arguments;

               platform::size::type instances = 1;
               std::vector< std::string> memberships;
               Environment environment;

               Lifetime lifetime;
               std::string note;

               inline friend bool operator == ( const Entity& lhs, const std::string& rhs) noexcept { return lhs.alias == rhs;}
               
               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( alias);
                  CASUAL_SERIALIZE( path);
                  CASUAL_SERIALIZE( arguments);
                  CASUAL_SERIALIZE( instances);
                  CASUAL_SERIALIZE( memberships);
                  CASUAL_SERIALIZE( environment);
                  CASUAL_SERIALIZE( lifetime);
                  CASUAL_SERIALIZE( note);
               )

               inline auto tie() const { return std::tie( path, alias, note, arguments, instances, memberships, environment, lifetime);}
            };
         } // detail

         struct Executable : detail::Entity
         {
 
         };

         struct Server : detail::Entity
         {

         };

         struct Model : common::Compare< Model>
         {
            std::string name;
            domain::Environment environment;
            std::vector< domain::Group> groups;
            std::vector< domain::Server> servers;
            std::vector< domain::Executable> executables;

            Model set_union( Model lhs, Model rhs);
            Model set_difference( Model lhs, Model rhs);
            Model set_intersection( Model lhs, Model rhs);
            
            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( name);
               CASUAL_SERIALIZE( environment);
               CASUAL_SERIALIZE( groups);
               CASUAL_SERIALIZE( servers);
               CASUAL_SERIALIZE( executables);
            )

            inline auto tie() const 
            { 
               return std::tie( name, environment, groups, servers, executables);
            }
         };

      } // domain


      namespace service
      {
         namespace restriction
         {
            struct Server : common::Compare< Server>
            {
               std::string alias;
               //! a set of regex, to match for allowed services. If empty all advertised services are allowed for the alias
               std::vector< std::string> services;
 
               inline friend bool operator == ( const Server& lhs, const std::string& alias) { return lhs.alias == alias;} 

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( alias);
                  CASUAL_SERIALIZE( services);
               )

               inline auto tie() const { return std::tie( alias, services);}
            };

         } // restriction
         
         struct Restriction : common::Compare< Restriction>
         {
            std::vector< restriction::Server> servers;

            Restriction set_union( Restriction lhs, Restriction rhs);
            Restriction set_difference( Restriction lhs, Restriction rhs);
            Restriction set_intersection( Restriction lhs, Restriction rhs);

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( servers);

            )
            inline auto tie() const { return std::tie( servers);}
         };

         struct Timeout : common::Compare< Timeout>
         {
            using Contract = common::service::execution::timeout::contract::Type;
            
            std::optional< platform::time::unit> duration;
            Contract contract = Contract::linger;

            Timeout set_union( Timeout lhs, Timeout rhs);

            inline explicit operator bool() const noexcept { return duration.has_value();}

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( duration);
               CASUAL_SERIALIZE( contract);
            )

            inline auto tie() const { return std::tie( duration, contract);}
         };

         struct Service : common::Compare< Service>
         {
            std::string name;
            std::vector< std::string> routes;
            service::Timeout timeout;
            std::optional< common::service::visibility::Type> visibility;
            std::string note;

            inline friend bool operator == ( const Service& lhs, const std::string& name) { return lhs.name == name;}

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( name);
               CASUAL_SERIALIZE( routes);
               CASUAL_SERIALIZE( timeout);
               CASUAL_SERIALIZE( visibility);
               CASUAL_SERIALIZE( note);
            )

            inline auto tie() const { return std::tie( name, routes, timeout, visibility, note);}
         };

         struct Global : common::Compare< Global>
         {
            service::Timeout timeout;
            std::string note;

            Global set_union( Global lhs, Global rhs);

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( timeout);
               CASUAL_SERIALIZE( note);
            )

            inline auto tie() const { return std::tie( timeout, note);}
         };

         struct Model : common::Compare< Model>
         {  
            //! "domain global" settings, for services that are not explicitly configured.
            Global global;

            std::vector< service::Service> services;
            Restriction restriction;

            Model set_union( Model lhs, Model rhs);
            Model set_difference( Model lhs, Model rhs);
            Model set_intersection( Model lhs, Model rhs);

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( global);
               CASUAL_SERIALIZE( services);
               CASUAL_SERIALIZE( restriction);
            )

            inline auto tie() const { return std::tie( global, services, restriction);}
         };
      } // service

      namespace transaction
      {
         struct Resource : common::Compare< Resource>
         {
            std::string name;
            std::string key;
            platform::size::type instances = 1;
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

            inline auto tie() const { return std::tie( name, key, instances, note, openinfo, closeinfo);}
         };

         //! TODO: maintainence - better name
         struct Mapping : common::Compare< Mapping>
         {
            std::string alias;
            std::vector< std::string> resources;

            inline friend bool operator == ( const Mapping& lhs, const std::string& alias) { return lhs.alias == alias;} 

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( alias);
               CASUAL_SERIALIZE( resources);
            )

            inline auto tie() const { return std::tie( alias, resources);}
         };

         struct Model : common::Compare< Model>
         {
            std::string log;
            std::vector< transaction::Resource> resources;
            std::vector< Mapping> mappings;

            Model set_union( Model lhs, Model rhs);
            Model set_difference( Model lhs, Model rhs);
            Model set_intersection( Model lhs, Model rhs);

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( log);
               CASUAL_SERIALIZE( resources);
               CASUAL_SERIALIZE( mappings);
            )

            inline auto tie() const { return std::tie( log, resources, mappings);}
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
            
            struct Connection : common::Compare< Connection>
            {
               std::string address;
               connection::discovery::Directive discovery{};
               std::string note;

               Connection set_union( Connection lhs, Connection rhs);

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( address);
                  CASUAL_SERIALIZE( discovery);
                  CASUAL_SERIALIZE( note);
               )

               inline auto tie() const { return std::tie( address, discovery, note);}
            };

            struct Limit : common::Compare< Limit>
            {
               platform::size::type size{};
               platform::size::type messages{};

               constexpr operator bool() const noexcept { return size > 0 || messages > 0;}

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( size);
                  CASUAL_SERIALIZE( messages);
               )

               inline auto tie() const { return std::tie( size, messages);}
            };

            struct Group : common::Compare< Group>
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

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( connect);
                  CASUAL_SERIALIZE( alias);
                  CASUAL_SERIALIZE( limit);
                  CASUAL_SERIALIZE( connections);
                  CASUAL_SERIALIZE( note);
               )

               inline auto tie() const { return std::tie( connect, alias, limit, connections, note);}
            };

         } // inbound

         struct Inbound : common::Compare< Inbound>
         {
            std::vector< inbound::Group> groups;

            inline bool empty() const { return groups.empty();}

            Inbound set_union( Inbound lhs, Inbound rhs);
            Inbound set_difference( Inbound lhs, Inbound rhs);
            Inbound set_intersection( Inbound lhs, Inbound rhs);

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( groups);
            )

            inline auto tie() const { return std::tie( groups);}
         };

         namespace outbound
         {
            struct Connection : common::Compare< Connection>
            {
               std::string address;
               std::vector< std::string> services;
               std::vector< std::string> queues;
               std::string note;

               Connection set_union( Connection lhs, Connection rhs);
               Connection set_difference( Connection lhs, Connection rhs);
               Connection set_intersection( Connection lhs, Connection rhs);

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( note);
                  CASUAL_SERIALIZE( address);
                  CASUAL_SERIALIZE( services);
                  CASUAL_SERIALIZE( queues);
               )

               inline explicit operator bool () const { return ! ( services.empty() && queues.empty());}

               inline auto tie() const { return std::tie( address, services, queues, note);}
            };

            struct Group : common::Compare< Group>
            {
               std::string alias;
               connect::Semantic connect = connect::Semantic::unknown;
               std::vector< outbound::Connection> connections;
               std::string note;

               Group set_union( Group lhs, Group rhs);
               Group set_difference( Group lhs, Group rhs);
               Group set_intersection( Group lhs, Group rhs);

               inline bool empty() const { return connections.empty();}

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( alias);
                  CASUAL_SERIALIZE( connect);
                  CASUAL_SERIALIZE( connections);
                  CASUAL_SERIALIZE( note);
               )

               inline auto tie() const { return std::tie( alias, connect, connections, note);}
            };

         } // outbound

         struct Outbound : common::Compare< Outbound>
         {
            std::vector< outbound::Group> groups;

            inline bool empty() const { return groups.empty();}

            Outbound set_union( Outbound lhs, Outbound rhs);
            Outbound set_difference( Outbound lhs, Outbound rhs);
            Outbound set_intersection( Outbound lhs, Outbound rhs);

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( groups);
            )

            inline auto tie() const { return std::tie( groups);}
         };

         struct Model : common::Compare< Model>
         {
            Inbound inbound;
            Outbound outbound;

            Model set_union( Model lhs, Model rhs);
            Model set_difference( Model lhs, Model rhs);
            Model set_intersection( Model lhs, Model rhs);

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( inbound);
               CASUAL_SERIALIZE( outbound);
            )

            inline auto tie() const { return std::tie( inbound, outbound);}
         };
         
      } // gateway

      namespace queue
      {
   
         struct Queue : common::Compare< Queue>
         {
            struct Retry : common::Compare< Retry>
            {
               platform::size::type count{};
               platform::time::unit delay = platform::time::unit::zero();

               inline auto empty() const { return count == 0 && delay == platform::time::unit::zero();}

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( count);
                  CASUAL_SERIALIZE( delay);
               )

               inline auto tie() const { return std::tie( count, delay);}
            };

            std::string name;
            Retry retry;
            std::string note;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( name);
               CASUAL_SERIALIZE( retry);
               CASUAL_SERIALIZE( note);
            )

            inline auto tie() const { return std::tie( name, retry, note);}
         };

         struct Group : common::Compare< Group>
         {
            std::string alias;
            std::string queuebase;
            std::string note;
            std::vector< Queue> queues;
            std::string directory;

            inline friend bool operator == ( const Group& lhs, const std::string& alias) { return lhs.alias == alias;}

            Group set_union( Group lhs, Group rhs);
            Group set_difference( Group lhs, Group rhs);
            Group set_intersection( Group lhs, Group rhs);

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( alias);
               CASUAL_SERIALIZE( queuebase);
               CASUAL_SERIALIZE( note);
               CASUAL_SERIALIZE( queues);
               CASUAL_SERIALIZE( directory);
            )

            inline auto tie() const { return std::tie( alias, queuebase, note, queues);}
            
         };

         namespace forward
         {
            struct forward_base : common::Compare< forward_base>
            {
               std::string alias;
               std::string source;
               platform::size::type instances = 1;
               std::string note;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( alias);
                  CASUAL_SERIALIZE( source);
                  CASUAL_SERIALIZE( instances);
                  CASUAL_SERIALIZE( note);
               )

               inline auto tie() const { return std::tie( alias, source, instances, note);}
            };

            struct Queue : forward_base, common::Compare< Queue>
            {
               struct Target : common::Compare< Target>
               {
                  std::string queue;
                  platform::time::unit delay{};

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( queue);
                     CASUAL_SERIALIZE( delay);
                  )

                  inline auto tie() const { return std::tie( queue, delay);}
               };

               Target target;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  forward_base::serialize( archive);
                  CASUAL_SERIALIZE( target);
               )

               inline auto tie() const { return std::tuple_cat( forward_base::tie(), std::tie( target));}
            };

            struct Service : forward_base, common::Compare< Service>
            {
               struct Target : common::Compare< Target>
               {
                  std::string service;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( service);
                  )
                  inline auto tie() const { return std::tie( service);}
               };

               using Reply = Queue::Target;

               Target target;
               std::optional< Reply> reply;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  forward_base::serialize( archive);
                  CASUAL_SERIALIZE( target);
                  CASUAL_SERIALIZE( reply);
               )

               inline auto tie() const { return std::tuple_cat( forward_base::tie(), std::tie( target, reply));}
            };

            struct Group : common::Compare< Group>
            {
               std::string alias;
               std::vector< forward::Service> services;
               std::vector< forward::Queue> queues;
               std::string note;

               Group set_union( Group lhs, Group rhs);
               Group set_difference( Group lhs, Group rhs);
               Group set_intersection( Group lhs, Group rhs);

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( alias);
                  CASUAL_SERIALIZE( services);
                  CASUAL_SERIALIZE( queues);
                  CASUAL_SERIALIZE( note);
               )

               inline auto tie() const { return std::tie( alias, services, queues);}
            };
         } // forward

         struct Forward : common::Compare< Forward>
         {
            std::vector< forward::Group> groups;

            Forward set_union( Forward lhs, Forward rhs);
            Forward set_difference( Forward lhs, Forward rhs);
            Forward set_intersection( Forward lhs, Forward rhs);

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( groups);
            )

            inline auto tie() const { return std::tie( groups);}
         };

         struct Model : common::Compare< Model>
         {
            std::vector< queue::Group> groups;
            Forward forward;
            std::string note;

            Model set_union( Model lhs, Model rhs);
            Model set_difference( Model lhs, Model rhs);
            Model set_intersection( Model lhs, Model rhs);

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( groups);
               CASUAL_SERIALIZE( forward);
               CASUAL_SERIALIZE( note);
            )

            inline auto tie() const { return std::tie( groups, forward, note);}
         };

      } // queue

   } // model

   struct Model : common::Compare< Model>
   {
      model::system::Model system;
      model::domain::Model domain;
      model::transaction::Model transaction;               
      model::service::Model service;
      model::queue::Model queue;
      model::gateway::Model gateway;

      inline friend Model operator + ( Model lhs, Model rhs) { return set_union( lhs, rhs);}

      friend Model normalize( Model model);
      friend Model set_union( Model lhs, Model rhs);
      friend Model set_difference( Model lhs, Model rhs);
      friend Model set_intersection( Model lhs, Model rhs);

      CASUAL_CONST_CORRECT_SERIALIZE(
         CASUAL_SERIALIZE( system);
         CASUAL_SERIALIZE( domain);
         CASUAL_SERIALIZE( transaction);
         CASUAL_SERIALIZE( service);
         CASUAL_SERIALIZE( queue);
         CASUAL_SERIALIZE( gateway);
      )

      inline auto tie() const { return std::tie( system, domain, transaction, service, queue, gateway);}
   };



} // casual::configuration