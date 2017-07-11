//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_STATE_H_
#define CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_STATE_H_

#include "domain/manager/task.h"

#include "common/message/domain.h"
#include "common/message/pending.h"
#include "common/uuid.h"
#include "common/platform.h"
#include "common/process.h"
#include "common/id.h"

#include "common/event/dispatch.h"

#include "configuration/environment.h"


#include  "sf/namevaluepair.h"
#include  "sf/archive/archive.h"

#include <unordered_map>
#include <vector>


namespace casual
{

   namespace sf
   {
      namespace archive
      {
         template< typename I, typename T>
         void serialize( Reader& archive, common::id::basic< I, T>& value, const char* name)
         {
            I integer;
            archive >> name::value::pair::make( name, integer);
            value = common::id::basic< I, T>{ integer};
         }

         template< typename I, typename T>
         void serialize( Writer& archive, const common::id::basic< I, T>& value, const char* name)
         {
            archive << name::value::pair::make( name, value.underlaying());
         }
      } // archive
   } // sf

   namespace domain
   {
      namespace manager
      {
         struct State;

         namespace state
         {
            namespace internal
            {

               template< typename T>
               struct Id
               {
                  using id_type = common::id::basic< std::size_t, T>;

                  id_type id = id_type::next();

                  friend bool operator == ( const Id& lhs, id_type rhs) { return lhs.id == rhs;}
                  friend bool operator < ( const Id& lhs, const Id& rhs) { return lhs.id < rhs.id;}

                  inline static void set_next_id( id_type id)
                  {
                     id_type::next( id);
                  }

               private:
               };


            } // internal



            struct Group : internal::Id< Group>
            {
               Group() = default;

               Group( std::string name, std::vector< id_type> dependencies, std::string note = "")
                  : name( std::move( name)), note( std::move( note)), dependencies( std::move( dependencies)) {}


               std::string name;
               std::string note;

               std::vector< id_type> dependencies;
               std::vector< std::string> resources;


               friend bool operator == ( const Group& lhs, Group::id_type id) { return lhs.id == id;}
               friend bool operator == ( Group::id_type id, const Group& rhs) { return id == rhs.id;}

               friend bool operator == ( const Group& lhs, const std::string& name) { return lhs.name == name;}

               friend std::ostream& operator << ( std::ostream& out, const Group& value);

               struct boot
               {
                  struct Order
                  {
                     bool operator () ( const Group& lhs, const Group& rhs);
                  };
               };

               //!
               //! For persistent state
               //!
               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  archive & CASUAL_MAKE_NVP( id);
                  archive & CASUAL_MAKE_NVP( name);
                  archive & CASUAL_MAKE_NVP( note);
                  archive & CASUAL_MAKE_NVP( dependencies);
                  archive & CASUAL_MAKE_NVP( resources);
               )
            };


            struct Process : internal::Id< Process>
            {
               typedef common::platform::pid::type pid_type;

               std::string alias;
               std::string path;
               std::vector< std::string> arguments;
               std::string note;

               std::vector< Group::id_type> memberships;

               struct
               {
                  std::vector< std::string> variables;
               } environment;


               bool restart = false;

               //!
               //! Number of instances that has been restarted
               //!
               std::size_t restarts = 0;

               //!
               //! For persistent state
               //!
               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  archive & CASUAL_MAKE_NVP( id);
                  archive & CASUAL_MAKE_NVP( alias);
                  archive & CASUAL_MAKE_NVP( path);
                  archive & CASUAL_MAKE_NVP( arguments);
                  archive & CASUAL_MAKE_NVP( note);

                  archive & CASUAL_MAKE_NVP( memberships);
                  archive & sf::name::value::pair::make(  "environment_variables", environment.variables);
                  archive & CASUAL_MAKE_NVP( restart);
                  archive & CASUAL_MAKE_NVP( restarts);
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

               void spawned( common::platform::pid::type pid)
               {
                  policy_type::spawned( pid, *this);
               }

               friend bool operator == ( const Instance& lhs, const handle_type& rhs) { return lhs.handle == rhs;}
               friend bool operator < ( const Instance& lhs, const Instance& rhs) { return lhs.state < rhs.state;}



               friend std::ostream& operator << ( std::ostream& out, const Instance& value)
               {
                  return out << "{ handle: " << value.handle
                        << ", state: " << value.state
                        << '}';
               }
            };


            struct Executable : Process
            {
               struct instance_policy
               {
                  using handle_type = common::platform::pid::type;
                  using state_type = instance::State;

                  template< typename I>
                  static void spawned( common::platform::pid::type pid, I& instance)
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

               void scale( std::size_t instances);

               void remove( pid_type instance);


               //!
               //! @return true if number of instances >= configured_instances
               //!
               //bool complete() const;

               friend std::ostream& operator << ( std::ostream& out, const Executable& value);

            };

            struct Server : Process
            {
               struct instance_policy
               {
                  using handle_type = common::process::Handle;
                  using state_type = instance::State;

                  template< typename I>
                  static void spawned( common::platform::pid::type pid, I& instance)
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

               //!
               //! Resources bound explicitly to this executable (if it's a server)
               //!
               std::vector< std::string> resources;

               //!
               //! If it's a server, the service restrictions. What will, at most, be advertised.
               //!
               std::vector< std::string> restrictions;

               instances_range spawnable();
               const_instances_range spawnable() const;
               const_instances_range shutdownable() const;

               void scale( std::size_t instances);


               instance_type instance( common::platform::pid::type pid) const;
               instance_type remove( common::platform::pid::type pid);

               bool connect( common::process::Handle process);

               //!
               //! @return true if number of instances >= configured_instances
               //!
               //bool complete() const;

               friend std::ostream& operator << ( std::ostream& out, const Server& value);

               friend bool operator == ( const Server& lhs, common::platform::pid::type rhs);

               //!
               //! For persistent state
               //!
               template< typename A>
               void serialize( A& archive)
               {
                  Process::serialize( archive);

                  archive & CASUAL_MAKE_NVP( resources);
                  archive & CASUAL_MAKE_NVP( restrictions);

                  auto instances_count = instances.size();
                  archive & CASUAL_MAKE_NVP( instances_count);
                  instances.resize( instances_count);
               }

               template< typename A>
               void serialize( A& archive) const
               {
                  Process::serialize( archive);

                  archive & CASUAL_MAKE_NVP( resources);
                  archive & CASUAL_MAKE_NVP( restrictions);

                  auto instances_count = instances.size();
                  archive & CASUAL_MAKE_NVP( instances_count);
               }
            };

            struct Batch
            {
               Batch( Group::id_type group);

               Group::id_type group;

               std::vector< Executable::id_type> executables;
               std::vector< Server::id_type> servers;

               friend std::ostream& operator << ( std::ostream& out, const Batch& value);
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

            std::map< common::Uuid, common::process::Handle> singeltons;

            struct
            {
               std::vector< common::message::pending::Message> replies;
               std::vector< common::message::domain::process::lookup::Request> lookup;
            } pending;


            //!
            //! check if task are done, and if so, start the next task
            //!
            bool execute();
            task::Queue tasks;


            //!
            //! executable id of this domain manager
            //!
            state::Server::id_type manager_id;

            //!
            //! Group id:s
            //!
            struct
            {
               using id_type = state::Group::id_type;

               id_type master;
               id_type transaction;
               id_type queue;

               id_type global;

               id_type gateway;

               //!
               //! For persistent state
               //!
               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  archive & CASUAL_MAKE_NVP( master);
                  archive & CASUAL_MAKE_NVP( transaction);
                  archive & CASUAL_MAKE_NVP( queue);
                  archive & CASUAL_MAKE_NVP( global);
                  archive & CASUAL_MAKE_NVP( gateway);
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


            //!
            //! global/default environment variables
            //!
            casual::configuration::Environment environment;

            //!
            //! this domain's original configuration.
            //!
            common::message::domain::configuration::Domain configuration;

            //!
            //! Runlevel can only "go forward"
            //!
            //! @{
            inline Runlevel runlevel() const { return m_runlevel;}
            void runlevel( Runlevel runlevel);
            //! @}

            std::vector< state::Batch> bootorder();
            std::vector< state::Batch> shutdownorder();

            //!
            //! Cleans up an exit (server or executable).
            //!
            //! @param pid
            //! @return pointer to Server and Executable which is not null if we gonna restart them.
            //!
            std::tuple< state::Server*, state::Executable*> exited( common::platform::pid::type pid);

            //!
            //! @return environment variables for the process, including global/default variables
            //!
            std::vector< std::string> variables( const state::Process& process);



            state::Group& group( state::Group::id_type id);
            const state::Group& group( state::Group::id_type id) const;

            state::Server* server( common::platform::pid::type pid);
            state::Server& server( state::Server::id_type id);
            const state::Server& server( state::Server::id_type id) const;

            state::Executable* executable( common::platform::pid::type pid);
            state::Executable& executable( state::Executable::id_type id);
            const state::Executable& executable( state::Executable::id_type id) const;


            common::process::Handle singleton( const common::Uuid& id) const;



            //!
            //! Extract all resources (names) configured to a specific process (server)
            //!
            //! @param pid process id
            //! @return resource names
            //!
            std::vector< std::string> resources( common::platform::pid::type pid);


            friend std::ostream& operator << ( std::ostream& out, const State& state);


            //!
            //! For persistent state
            //!
            CASUAL_CONST_CORRECT_SERIALIZE
            (
               archive & CASUAL_MAKE_NVP( manager_id);
               archive & CASUAL_MAKE_NVP( groups);
               archive & CASUAL_MAKE_NVP( servers);
               archive & CASUAL_MAKE_NVP( executables);
               archive & CASUAL_MAKE_NVP( group_id);
               archive & CASUAL_MAKE_NVP( environment);
               archive & CASUAL_MAKE_NVP( configuration);
            )

            bool mandatory_prepare = true;
            bool auto_persist = true;

         private:
            Runlevel m_runlevel = Runlevel::startup;


         };


      } // manager
   } // domain
} // casual

#endif // CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_STATE_H_
