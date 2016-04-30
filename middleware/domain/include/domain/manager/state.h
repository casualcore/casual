//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_STATE_H_
#define CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_STATE_H_

#include "common/message/type.h"
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

               private:
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

               struct Resource : internal::Id< Resource>
               {
                  Resource() = default;
                  Resource( std::size_t instances, const std::string& key, const std::string& openinfo, const std::string& closeinfo)
                  : instances{ instances}, key{ key}, openinfo{ openinfo}, closeinfo{ closeinfo} {}


                  std::size_t instances;
                  std::string key;
                  std::string openinfo;
                  std::string closeinfo;

                  CASUAL_CONST_CORRECT_SERIALIZE({
                     archive & CASUAL_MAKE_NVP( instances);
                     archive & CASUAL_MAKE_NVP( key);
                     archive & CASUAL_MAKE_NVP( openinfo);
                     archive & CASUAL_MAKE_NVP( closeinfo);
                  })

                  friend std::ostream& operator << ( std::ostream& out, const Resource& value);
               };


               std::string name;
               std::string note;

               std::vector< Resource> resources;
               std::vector< id_type> dependencies;

               CASUAL_CONST_CORRECT_SERIALIZE({
                  archive & CASUAL_MAKE_NVP( id);
                  archive & CASUAL_MAKE_NVP( name);
                  archive & CASUAL_MAKE_NVP( note);
                  archive & CASUAL_MAKE_NVP( resources);
                  archive & CASUAL_MAKE_NVP( dependencies);
               })

               friend bool operator == ( const Group& lhs, Group::id_type id) { return lhs.id == id;}
               friend bool operator == ( Group::id_type id, const Group& rhs) { return id == rhs.id;}

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


               bool remove( pid_type instance);
               bool offline() const;
               bool online() const;

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
            std::vector< common::process::Handle> mandatory;

            processes_type processes;

            std::map< common::Uuid, common::process::Handle> singeltons;

            struct
            {
               std::vector< common::message::pending::Message> replies;
               std::vector< common::message::process::lookup::Request> lookup;
            } pending;

            struct
            {
               std::vector< common::process::Handle> listeners;
            } termination;

            Runlevel runlevel = Runlevel::startup;

            std::vector< state::Batch> bootorder();
            std::vector< state::Batch> shutdownorder();

            void remove_process( common::platform::pid::type pid);


         };

      } // manager
   } // domain
} // casual

#endif // CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_STATE_H_
