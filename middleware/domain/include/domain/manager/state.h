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


#include  "sf/namevaluepair.h"

#include <unordered_map>
#include <vector>


namespace casual
{

   namespace domain
   {
      namespace manager
      {

         namespace state
         {
            namespace internal
            {
               template< typename T>
               struct Id
               {
                  using id_type = std::size_t;

                  id_type id = nextId();

                  friend bool operator == ( const Id& lhs, id_type rhs) { return lhs.id == rhs;}

                  inline static void set_next_id( id_type id)
                  {
                     m_next_id = id;
                  }

               private:

                  inline static id_type nextId()
                  {
                     return m_next_id++;
                  }

                  static id_type m_next_id;
               };

               template< typename T>
               std::size_t Id< T>::m_next_id = 10;

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



            struct Executable : internal::Id< Executable>
            {
               typedef common::platform::pid::type pid_type;

               std::string alias;
               std::string path;
               std::vector< std::string> arguments;
               std::string note;

               std::vector< pid_type> instances;

               std::vector< Group::id_type> memberships;

               struct
               {
                  std::vector< std::string> variables;
               } environment;


               std::size_t configured_instances = 0;

               bool restart = false;

               //!
               //! Number of instances that has been restarted
               //!
               std::size_t restarts = 0;

               //!
               //! Resources bound explicitly to this executable (if it's a server)
               //!
               std::vector< std::string> resources;

               //!
               //! If it's a server, the service restrictions. What will, at most, be advertised.
               //!
               std::vector< std::string> restrictions;


               bool remove( pid_type instance);
               bool offline() const;
               bool online() const;

               //!
               //! @return true if number of instances >= configured_instances
               //!
               bool complete() const;

               friend std::ostream& operator << ( std::ostream& out, const Executable& value);



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
                  archive & CASUAL_MAKE_NVP( configured_instances);
                  archive & CASUAL_MAKE_NVP( restart);
                  archive & CASUAL_MAKE_NVP( restarts);
                  archive & CASUAL_MAKE_NVP( resources);
                  archive & CASUAL_MAKE_NVP( restrictions);

               )

            };

            struct Batch
            {
               Batch( const Group&);
               //Batch( const Batch&) = default;


               std::reference_wrapper< const Group> group;
               std::vector< std::reference_wrapper< Executable>> executables;
               std::chrono::milliseconds timeout() const;

               bool online() const;
               bool offline() const;

               friend std::ostream& operator << ( std::ostream& out, const Batch& value);
            };

            namespace configuration
            {



            } // configuration

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

            using processes_type = std::unordered_map< common::platform::pid::type, common::process::Handle>;

            std::vector< state::Group> groups;
            std::vector< state::Executable> executables;


            processes_type processes;

            std::map< common::Uuid, common::process::Handle> singeltons;

            struct
            {
               std::vector< common::message::pending::Message> replies;
               std::vector< common::message::domain::process::lookup::Request> lookup;
            } pending;

            struct
            {
               std::vector< common::process::Handle> listeners;
            } termination;



            //!
            //! check if task are done, and if so, start the next task
            //!
            bool execute();
            task::Queue tasks;


            //!
            //! executable id of this domain manager
            //!
            state::Executable::id_type manager_id = 0;

            //!
            //! Group id:s
            //!
            struct
            {
               using id_type = state::Group::id_type;

               id_type master = 0;
               id_type transaction = 0;
               id_type queue = 0;

               id_type global = 0;

               id_type gateway = 0;

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

            void remove_process( common::platform::pid::type pid);


            state::Executable& executable( common::platform::pid::type pid);
            state::Group& group( state::Group::id_type id);
            const state::Group& group( state::Group::id_type id) const;



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
               archive & CASUAL_MAKE_NVP( executables);
               archive & CASUAL_MAKE_NVP( group_id);
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
