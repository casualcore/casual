//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "domain/manager/task.h"
#include "domain/strong/id.h"

#include "casual/platform.h"

#include "common/message/domain.h"
#include "common/message/pending.h"
#include "common/uuid.h"
#include "common/state/machine.h"
#include "common/process.h"

#include "common/event/dispatch.h"

#include "configuration/model.h"

#include "common/serialize/macro.h"

#include <unordered_map>
#include <vector>


namespace casual
{
   namespace domain::manager
   {
      struct State;

      namespace state
      {

         struct Group
         {
            using id_type = strong::group::id;
            Group() = default;

            Group( std::string name, std::vector< id_type> dependencies, std::string note = "")
               : name( std::move( name)), note( std::move( note)), dependencies( std::move( dependencies)) {}

            id_type id = id_type::generate();

            std::string name;
            std::string note;

            std::vector< id_type> dependencies;
            
            //! only to help with TM configuration.
            std::vector< std::string> resources;


            friend bool operator == ( const Group& lhs, Group::id_type id) { return lhs.id == id;}
            friend bool operator == ( Group::id_type id, const Group& rhs) { return id == rhs.id;}
            friend bool operator == ( const Group& lhs, const std::string& name) { return lhs.name == name;}

            struct boot
            {
               struct Order
               {
                  bool operator () ( const Group& lhs, const Group& rhs);
               };
            };

            inline friend bool operator < ( const Group& l, const Group& r) { return l.id < r.id;}

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( id);
               CASUAL_SERIALIZE( name);
               CASUAL_SERIALIZE( note);
               CASUAL_SERIALIZE( dependencies);
            )
         };

         //! TODO maintainability: process is not a good name

         template< typename ID>
         struct Process 
         {
            using id_type = ID;

            id_type id = id_type::generate();

            std::string alias;
            std::string path;
            std::vector< std::string> arguments;
            std::string note;

            std::vector< Group::id_type> memberships;

            struct
            {
               std::vector< common::environment::Variable> variables;
               
               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( variables);
               )
            } environment;

            bool restart = false;

            //! Number of instances that has been restarted
            platform::size::type restarts = 0;

            inline friend bool operator < ( const Process& l, const Process& r) { return l.id < r.id;}

            CASUAL_LOG_SERIALIZE
            (
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
         };

         namespace instance
         {
            enum class State : short
            {
               running,
               spawned,
               scale_out,
               scale_in,
               exit,
               error,
            };

            std::ostream& operator << ( std::ostream& out, State value);

         } // instance

         template< typename P>
         struct Instance
         {
            using policy_type = P;
            using handle_type = typename policy_type::handle_type;
            using state_type = typename policy_type::state_type;

            handle_type handle;
            state_type state = state_type::scale_out;
            platform::time::point::type spawnpoint = platform::time::point::limit::zero();
            
            void spawned( common::strong::process::id pid)
            {
               policy_type::spawned( pid, *this);
               spawnpoint = platform::time::clock::type::now();
            }

            friend bool operator == ( const Instance& lhs, common::strong::process::id pid) { return lhs.handle == pid;}
            friend bool operator == ( common::strong::process::id pid, const Instance& rhs) { return pid == rhs.handle;}

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( handle);
               CASUAL_SERIALIZE( state);
               CASUAL_SERIALIZE( spawnpoint);
            )
         };


         struct Executable : Process< strong::executable::id> 
         {
            struct instance_policy
            {
               using handle_type = common::strong::process::id;
               using state_type = instance::State;

               template< typename I>
               static void spawned( common::strong::process::id pid, I& instance)
               {
                  instance.handle = pid;
                  instance.state = state_type::running;
               }

            };

            using instance_type = Instance< instance_policy>;
            using state_type = typename instance_type::state_type;

            using instances_range = common::range::type_t< std::vector< instance_type>>;
            using const_instances_range = common::range::type_t< const std::vector< instance_type>>;

            std::vector< instance_type> instances;

            instances_range spawnable();
            const_instances_range spawnable() const;
            const_instances_range shutdownable() const;

            void scale( platform::size::type instances);
            void remove( common::strong::process::id instance);

            friend bool operator == ( const Executable& lhs, common::strong::process::id rhs);
            inline friend bool operator == ( common::strong::process::id lhs, const Executable& rhs) { return rhs == lhs;}

            CASUAL_LOG_SERIALIZE({
               Process::serialize( archive);
               CASUAL_SERIALIZE( instances);
            })
         };

         struct Server : Process< strong::server::id>
         {
            struct instance_policy
            {
               using handle_type = common::process::Handle;
               using state_type = instance::State;

               template< typename I>
               static void spawned( common::strong::process::id pid, I& instance)
               {
                  instance.handle.pid = pid;
                  instance.state = state_type::spawned;
               }
            };

            using instance_type = Instance< instance_policy>;
            using state_type = typename instance_type::state_type;

            using instances_range = common::range::type_t< std::vector< instance_type>>;
            using const_instances_range = common::range::type_t< const std::vector< instance_type>>;

            std::vector< instance_type> instances;

            instances_range spawnable();
            const_instances_range spawnable() const;
            const_instances_range shutdownable() const;

            void scale( platform::size::type instances);

            const instance_type* instance( common::strong::process::id pid) const;

            //! @returns 'null handle' if not found
            common::process::Handle remove( common::strong::process::id pid);

            bool connect( const common::process::Handle& process);

            friend bool operator == ( const Server& lhs, common::strong::process::id rhs);
            inline friend bool operator == ( common::strong::process::id lhs, const Server& rhs) { return rhs == lhs;}

            CASUAL_LOG_SERIALIZE({
               Process::serialize( archive);;
               CASUAL_SERIALIZE( instances);
            })
         };

         namespace dependency
         {
            struct Group
            {
               std::string description;
               std::vector< Server::id_type> servers;
               std::vector< Executable::id_type> executables;

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( description);
                  CASUAL_SERIALIZE( executables);
                  CASUAL_SERIALIZE( servers);
               )
            };
            
         } // dependency


         namespace is
         {
            bool singleton( common::strong::process::id pid);
         } // is


         struct Parent
         {
            common::strong::ipc::id ipc;
            common::strong::correlation::id correlation;

            inline explicit operator bool() const noexcept { return common::predicate::boolean( ipc);}

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( ipc);
               CASUAL_SERIALIZE( correlation);
            )
         };

         enum class Runlevel : short
         {
            startup,
            running,
            shutdown,
            error,
         };
         std::ostream& operator << ( std::ostream& out, Runlevel value);

      } // state


      struct State
      {
         common::state::Machine< state::Runlevel> runlevel;

         std::vector< state::Server> servers;
         std::vector< state::Executable> executables;
         std::vector< state::Group> groups;

         std::map< common::Uuid, common::process::Handle> singletons;

         struct
         {
            std::vector< common::message::domain::process::lookup::Request> lookup;
         } pending;

         //! check if task are done, and if so, start the next task
         bool execute();
         task::Queue tasks;

         //! Processes that register but is not direct children of
         //! this process.
         std::vector< common::process::Handle> grandchildren;

         //! executable id of this domain manager
         state::Server::id_type manager_id;

         struct
         {
            //! process for casual-domain-pending-message
            //common::Process pending;

            //! process for casual-domain-discovery
            //common::Process discovery;
         } process;
         

         //! Group id:s
         struct
         {
            using id_type = state::Group::id_type;
            
            id_type core;
            id_type master;
            id_type transaction;
            id_type queue;

            id_type global;

            id_type gateway;

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( core);
               CASUAL_SERIALIZE( master);
               CASUAL_SERIALIZE( transaction);
               CASUAL_SERIALIZE( queue);
               CASUAL_SERIALIZE( global);
               CASUAL_SERIALIZE( gateway);
            )

         } group_id;

         common::event::dispatch::Collection<
            task::message::domain::Information,
            common::message::event::process::Spawn,
            common::message::event::process::Exit,
            common::message::event::process::Assassination,
            common::message::event::Task,
            common::message::event::sub::Task,
            common::message::event::Error,
            common::message::event::discoverable::Avaliable
         > event;

         std::vector< common::strong::process::id> whitelisted;

         state::Parent parent;

         //! this domain's original configuration.
         casual::configuration::Model configuration;

         //! the 'singleton' file for the domain
         common::file::scoped::Path singelton;

         std::vector< state::dependency::Group> bootorder() const;
         std::vector< state::dependency::Group> shutdownorder() const;

         //! Cleans up an exit (server or executable).
         //!
         //! @param pid
         //! @return pointer to Server and Executable which is not null if we gonna restart them.
         std::tuple< state::Server*, state::Executable*> remove( common::strong::process::id pid);

         //! @return environment variables for the process, including global/default variables
         template< typename E>
         inline std::vector< common::environment::Variable> variables( const E& executable)
         {
            return variables( executable.environment.variables);
         }

         state::Group& group( state::Group::id_type id);
         const state::Group& group( state::Group::id_type id) const;

         state::Server* server( common::strong::process::id pid) noexcept;
         const state::Server* server( common::strong::process::id pid) const noexcept;
         state::Executable* executable( common::strong::process::id pid) noexcept;
         const state::Executable* executable( common::strong::process::id pid) const noexcept;


         state::Server& entity( state::Server::id_type id);
         const state::Server& entity( state::Server::id_type id) const;
         state::Executable& entity( state::Executable::id_type id);
         const state::Executable& entity( state::Executable::id_type id) const;

         struct Runnables
         {
            std::vector< std::reference_wrapper< state::Server>> servers;
            std::vector< std::reference_wrapper< state::Executable>> executables;
            CASUAL_LOG_SERIALIZE({
               CASUAL_SERIALIZE( servers);
               CASUAL_SERIALIZE( executables);
            })
         };

         Runnables runnables( std::vector< std::string> aliases);

         common::process::Handle grandchild( common::strong::process::id pid) const noexcept;

         common::process::Handle singleton( const common::Uuid& id) const noexcept;

         //! @return all 'running' id:s of 'aliases' that are untouchable, ie. internal casual stuff.
         std::tuple< std::vector< state::Server::id_type>, std::vector< state::Executable::id_type>> untouchables() const noexcept;


         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE_NAME( runlevel(), "runlevel");
            CASUAL_SERIALIZE( manager_id);
            CASUAL_SERIALIZE( groups);
            CASUAL_SERIALIZE( servers);
            CASUAL_SERIALIZE( executables);
            CASUAL_SERIALIZE( group_id);
            CASUAL_SERIALIZE( whitelisted);
            CASUAL_SERIALIZE( parent);
            CASUAL_SERIALIZE( configuration);
            CASUAL_SERIALIZE( bare);
         )

         bool bare = false;

      private:
         std::vector< common::environment::Variable> variables( const std::vector< common::environment::Variable>& variables);
      
      };

   } // domain::manager
} // casual


