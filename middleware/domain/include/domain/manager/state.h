//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "domain/manager/task.h"

#include "common/message/domain.h"
#include "common/message/pending.h"
#include "common/uuid.h"
#include "common/platform.h"
#include "common/process.h"
#include "common/value/id.h"

#include "common/event/dispatch.h"

#include "configuration/environment.h"

#include "common/serialize/macro.h"

#include <unordered_map>
#include <vector>


namespace casual
{

   namespace domain
   {
      namespace manager
      {
         struct State;

         namespace state
         {
            using size_type = common::platform::size::type;


            namespace id
            {
               template< typename Tag>
               using type = common::value::basic_id< size_type, common::value::id::policy::unique_initialize< size_type, Tag, 0>>;
               
            } // id

            struct Group
            {
               using id_type = id::type< Group>;
               Group() = default;

               Group( std::string name, std::vector< id_type> dependencies, std::string note = "")
                  : name( std::move( name)), note( std::move( note)), dependencies( std::move( dependencies)) {}

               id_type id;

               std::string name;
               std::string note;

               std::vector< id_type> dependencies;
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

               //! For persistent state
               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  CASUAL_SERIALIZE( id);
                  CASUAL_SERIALIZE( name);
                  CASUAL_SERIALIZE( note);
                  CASUAL_SERIALIZE( dependencies);
                  CASUAL_SERIALIZE( resources);
               )
            };


            struct Process 
            {
               using id_type = id::type< Group>;
               using pid_type = common::strong::process::id;

               id_type id;

               std::string alias;
               std::string path;
               std::vector< std::string> arguments;
               std::string note;

               std::vector< Group::id_type> memberships;

               struct
               {
                  std::vector< common::environment::Variable> variables;
               } environment;

               bool restart = false;

               //! Number of instances that has been restarted
               size_type restarts = 0;

               inline friend bool operator < ( const Process& l, const Process& r) { return l.id < r.id;}

               //! For persistent state
               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  CASUAL_SERIALIZE( id);
                  CASUAL_SERIALIZE( alias);
                  CASUAL_SERIALIZE( path);
                  CASUAL_SERIALIZE( arguments);
                  CASUAL_SERIALIZE( note);

                  CASUAL_SERIALIZE( memberships);
                  CASUAL_SERIALIZE_NAME( environment.variables, "environment_variables");
                  CASUAL_SERIALIZE( restart);
                  CASUAL_SERIALIZE( restarts);
               )
            };

            namespace instance
            {
               enum class State : int
               {
                  running,
                  scale_out,
                  scale_in,
                  exit,
                  spawn_error,
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

               void spawned( common::strong::process::id pid)
               {
                  policy_type::spawned( pid, *this);
               }

               friend bool operator == ( const Instance& lhs, common::strong::process::id pid) { return lhs.handle == pid;}
               friend bool operator < ( const Instance& lhs, const Instance& rhs) { return lhs.state < rhs.state;}

               CASUAL_CONST_CORRECT_SERIALIZE({
                  CASUAL_SERIALIZE( handle);
                  CASUAL_SERIALIZE( state);
               })
            };


            struct Executable : Process
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

               void scale( size_type instances);
               void remove( pid_type instance);

               CASUAL_CONST_CORRECT_SERIALIZE({
                  Process::serialize( archive);
                  CASUAL_SERIALIZE( instances);
               })
            };

            struct Server : Process
            {
               struct instance_policy
               {
                  using handle_type = common::process::Handle;
                  using state_type = instance::State;

                  template< typename I>
                  static void spawned( common::strong::process::id pid, I& instance)
                  {
                     instance.handle.pid = pid;
                     instance.state = state_type::scale_out;
                  }
               };

               using instance_type = Instance< instance_policy>;
               using state_type = typename instance_type::state_type;

               using instances_range = common::range::type_t< std::vector< instance_type>>;
               using const_instances_range = common::range::type_t< const std::vector< instance_type>>;

               std::vector< instance_type> instances;

               //! Resources bound explicitly to this executable (if it's a server)
               std::vector< std::string> resources;

               //! If it's a server, the service restrictions. What will, at most, be advertised.
               std::vector< std::string> restrictions;

               instances_range spawnable();
               const_instances_range spawnable() const;
               const_instances_range shutdownable() const;

               void scale( size_type instances);


               instance_type instance( common::strong::process::id pid) const;
               instance_type remove( common::strong::process::id pid);

               bool connect( common::process::Handle process);

               friend bool operator == ( const Server& lhs, common::strong::process::id rhs);

               //! For persistent state
               template< typename A>
               void serialize( A& archive)
               {
                  Process::serialize( archive);

                  CASUAL_SERIALIZE( resources);
                  CASUAL_SERIALIZE( restrictions);

                  auto instances_count = instances.size();
                  CASUAL_SERIALIZE( instances_count);
                  instances.resize( instances_count);
               }

               template< typename A>
               void serialize( A& archive) const
               {
                  Process::serialize( archive);

                  CASUAL_SERIALIZE( resources);
                  CASUAL_SERIALIZE( restrictions);

                  auto instances_count = instances.size();
                  CASUAL_SERIALIZE( instances_count);
               }
            };

            struct Batch
            {
               Batch( Group::id_type group);

               Group::id_type group;

               std::vector< Executable::id_type> executables;
               std::vector< Server::id_type> servers;

               void log( std::ostream& out, const State& state) const;

               CASUAL_CONST_CORRECT_SERIALIZE_WRITE({
                  CASUAL_SERIALIZE( executables);
                  CASUAL_SERIALIZE( servers);
               })
            };
            static_assert( common::traits::is_movable< Batch>::value, "not movable");


         } // state


         struct State
         {
            enum class Runlevel
            {
               startup,
               running,
               shutdown,
               error,
            };

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
               common::Process pending;
            } process;
           

            //! Group id:s
            struct
            {
               using id_type = state::Group::id_type;

               id_type master;
               id_type transaction;
               id_type queue;

               id_type global;

               id_type gateway;

               //! For persistent state
               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  CASUAL_SERIALIZE( master);
                  CASUAL_SERIALIZE( transaction);
                  CASUAL_SERIALIZE( queue);
                  CASUAL_SERIALIZE( global);
                  CASUAL_SERIALIZE( gateway);
               )

            } group_id;

            common::event::dispatch::Collection<
               common::message::event::process::Spawn,
               common::message::event::process::Exit,
               common::message::event::domain::boot::Begin,
               common::message::event::domain::boot::End,
               common::message::event::domain::shutdown::Begin,
               common::message::event::domain::shutdown::End,
               common::message::event::domain::Group,
               common::message::event::domain::Error,
               common::message::event::domain::server::Connect
            > event;

            //! global/default environment variables
            casual::configuration::Environment environment;

            //! this domain's original configuration.
            common::message::domain::configuration::Domain configuration;

            //! Runlevel can only "go forward"
            //! @{
            inline Runlevel runlevel() const noexcept { return m_runlevel;}
            void runlevel( Runlevel runlevel) noexcept;
            //! @}

            std::vector< state::Batch> bootorder();
            std::vector< state::Batch> shutdownorder();

            //! Cleans up an exit (server or executable).
            //!
            //! @param pid
            //! @return pointer to Server and Executable which is not null if we gonna restart them.
            std::tuple< state::Server*, state::Executable*> remove( common::strong::process::id pid);

            //! @return environment variables for the process, including global/default variables
            std::vector< common::environment::Variable> variables( const state::Process& process);

            state::Group& group( state::Group::id_type id);
            const state::Group& group( state::Group::id_type id) const;

            state::Server* server( common::strong::process::id pid) noexcept;
            const state::Server* server( common::strong::process::id pid) const noexcept;
            state::Server& server( state::Server::id_type id);
            const state::Server& server( state::Server::id_type id) const;

            state::Executable* executable( common::strong::process::id pid) noexcept;
            state::Executable& executable( state::Executable::id_type id);
            const state::Executable& executable( state::Executable::id_type id) const;

            common::process::Handle grandchild( common::strong::process::id pid) const noexcept;

            common::process::Handle singleton( const common::Uuid& id) const noexcept;

            //! Extract all resources (names) configured to a specific process (server)
            //!
            //! @param pid process id
            //! @return resource names
            std::vector< std::string> resources( common::strong::process::id pid);


            //! For persistent state
            CASUAL_CONST_CORRECT_SERIALIZE
            (
               CASUAL_SERIALIZE( manager_id);
               CASUAL_SERIALIZE( groups);
               CASUAL_SERIALIZE( servers);
               CASUAL_SERIALIZE( executables);
               CASUAL_SERIALIZE( group_id);
               CASUAL_SERIALIZE( environment);
               CASUAL_SERIALIZE( configuration);
            )

            bool mandatory_prepare = true;
            bool persist = true;

         private:
            Runlevel m_runlevel = Runlevel::startup;
         };

      } // manager
   } // domain
} // casual


