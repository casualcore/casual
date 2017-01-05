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

#include "configuration/message/domain.h"

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

               private:

                  //id_type m_id = nextId();

                  inline static id_type nextId()
                  {
                     static id_type id = 10;
                     return id++;
                  }
               };

            } // internal

            struct Group : internal::Id< Group>
            {
               Group() = default;
               Group( std::string name, std::string note = "") : name( std::move( name)), note( std::move( note)) {}

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


               bool remove( pid_type instance);
               bool offline() const;
               bool online() const;

               //!
               //! @return true if number of instances >= configured_instances
               //!
               bool complete() const;

               friend std::ostream& operator << ( std::ostream& out, const Executable& value);

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

            struct
            {
               state::Group::id_type group = 0;
               state::Group::id_type last = 0;

               //!
               //! executable id of this domain manager
               //!
               state::Executable::id_type manager = 0;

            } global;


            //!
            //! this domain's original configuration.
            //!
            configuration::message::Domain configuration;

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



            //!
            //! Extract all resources (names) configured to a specific process (server)
            //!
            //! @param pid process id
            //! @return resource names
            //!
            std::vector< std::string> resources( common::platform::pid::type pid);


            friend std::ostream& operator << ( std::ostream& out, const State& state);

         private:
            Runlevel m_runlevel = Runlevel::startup;


         };

      } // manager
   } // domain
} // casual

#endif // CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_STATE_H_
