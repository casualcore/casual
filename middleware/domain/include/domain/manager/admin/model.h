//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "common/serialize/macro.h"
#include "casual/platform.h"

#include "configuration/user/environment.h"

#include "common/domain.h"
#include "common/build.h"

namespace casual
{
   namespace domain::manager::admin::model
   {

      inline namespace v1
      {
         using id_type = platform::size::type;
         using size_type = platform::size::type;

         struct Group
         {
            id_type id;
            std::string name;
            std::string note;
            bool enabled = true;

            std::vector< id_type> dependencies;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( id);
               CASUAL_SERIALIZE( name);
               CASUAL_SERIALIZE( note);
               CASUAL_SERIALIZE( enabled);
               CASUAL_SERIALIZE( dependencies);
            )
         };

         struct Process
         {
            id_type id;
            std::string alias;
            std::filesystem::path path;
            std::vector< std::string> arguments;
            std::string note;

            std::vector< id_type> memberships;

            struct
            {
               std::vector< std::string> variables;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( variables);
               )

            } environment;

            bool restart = false;
            size_type restarts = 0;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( id);
               CASUAL_SERIALIZE( alias);
               CASUAL_SERIALIZE( path);
               CASUAL_SERIALIZE( arguments);
               CASUAL_SERIALIZE( note);
               CASUAL_SERIALIZE( memberships);
               CASUAL_SERIALIZE( environment);
               CASUAL_SERIALIZE( restart);
               CASUAL_SERIALIZE( restarts);
            )

            inline friend bool operator < ( const Process& lhs, const Process& rhs) { return lhs.id < rhs.id;}
            inline friend bool operator == ( const Process& lhs, std::string_view alias) { return lhs.alias == alias;}

         };

         namespace instance
         {
            enum class State : int
            {
               running,
               spawned,
               scale_out,
               scale_in,
               exit,
               error,
            };

            constexpr std::string_view description( State state)
            {
               switch( state)
               {
                  case State::running: return "running";
                  case State::spawned: return "spawned";
                  case State::scale_out: return "scale-out";
                  case State::scale_in: return "scale-in";
                  case State::exit: return "exit";
                  case State::error: return "error";
               }
               return "<unknown>";
            }
         } // instance

         template< typename H>
         struct Instance
         {
            H handle;
            instance::State state = instance::State::scale_out;
            platform::time::point::type spawnpoint;

            template< typename T>
            friend bool operator == ( const Instance& lhs, T&& rhs) 
            { 
               return common::process::id( lhs.handle) == common::process::id( rhs);
            }

            inline explicit operator bool() const noexcept { return common::predicate::boolean( handle);}

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( handle);
               CASUAL_SERIALIZE( state);
               CASUAL_SERIALIZE( spawnpoint);
            )
         };

         struct Executable : Process
         {
            using instance_type = Instance< common::strong::process::id>;
            std::vector< instance_type> instances;

            CASUAL_CONST_CORRECT_SERIALIZE(
               Process::serialize( archive);
               CASUAL_SERIALIZE( instances);
            )
         };

         struct Server : Process
         {
            using instance_type = Instance< common::process::Handle>;
            std::vector< instance_type> instances;

            std::vector< std::string> resources;
            std::vector< std::string> restriction;

            CASUAL_CONST_CORRECT_SERIALIZE(
               Process::serialize( archive);
               CASUAL_SERIALIZE( instances);
               CASUAL_SERIALIZE( resources);
               CASUAL_SERIALIZE( restriction);
            )
         };

         struct Task
         {
            common::strong::correlation::id id;
            std::string description;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( id);
               CASUAL_SERIALIZE( description);
            )
         };

         struct Grandchild
         {
            common::process::Handle handle;
            std::string alias;
            std::filesystem::path path;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( handle);
               CASUAL_SERIALIZE( alias);
               CASUAL_SERIALIZE( path);
            )
         };

         namespace state
         {
            enum class Runlevel : int
            {
               startup = 0,
               running = 1,
               shutdown = 2,
               error = 3,
            };

            constexpr std::string_view description( Runlevel runlevel)
            {
               switch( runlevel)
               {
                  case Runlevel::startup: return "startup";
                  case Runlevel::running: return "running";
                  case Runlevel::shutdown: return "shutdown";
                  case Runlevel::error: return "error";
               }
               return "<unknown>";
            }
         } // state

         struct State
         {
            state::Runlevel runlevel = state::Runlevel::startup;
            common::build::Version version; 
            common::domain::Identity identity;
            std::vector< model::Group> groups;
            std::vector< model::Executable> executables;
            std::vector< model::Server> servers;
            std::vector< model::Grandchild> grandchildren;

            struct Tasks
            {
               std::vector< model::Task> running;
               std::vector< model::Task> pending;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( running);
                  CASUAL_SERIALIZE( pending);
               )
            } tasks;

            struct
            {
               std::vector< common::process::Handle> listeners;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( listeners);
               )

            } event;

            CASUAL_CONST_CORRECT_SERIALIZE({
               CASUAL_SERIALIZE( runlevel);
               CASUAL_SERIALIZE( version);
               CASUAL_SERIALIZE( identity);
               CASUAL_SERIALIZE( groups);
               CASUAL_SERIALIZE( executables);
               CASUAL_SERIALIZE( servers);
               CASUAL_SERIALIZE( grandchildren);
               CASUAL_SERIALIZE( tasks);
               CASUAL_SERIALIZE( event);
            })

         };


         namespace scale
         {
            struct Alias
            {
               std::string name;
               size_type instances;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( name);
                  CASUAL_SERIALIZE( instances);
               )
            };

         } // scale

         namespace restart
         {
            struct Alias
            {
               std::string name;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( name);
               )
            };

            struct Group
            {
               std::string name;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( name);
               )
            };
            
         } // restart

         namespace set
         {
            struct Environment
            {
               std::vector< common::environment::Variable> variables;
               std::vector< std::string> aliases;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( variables);
                  CASUAL_SERIALIZE( aliases);
               )
            };
            
         } // set

         namespace unset
         {
            struct Environment
            {
               std::vector< std::string> variables;
               std::vector< std::string> aliases;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( variables);
                  CASUAL_SERIALIZE( aliases);
               )
            };
            
         } // unset
         
      } // v1
   } // domain::manager::admin::model
} // casual


