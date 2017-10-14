//!
//! casual
//!

#ifndef CASUAL_GATEWAY_MANAGER_ADMIN_VO_H
#define CASUAL_GATEWAY_MANAGER_ADMIN_VO_H


#include "sf/namevaluepair.h"
#include "sf/platform.h"

#include "common/domain.h"

namespace casual
{
   namespace domain
   {
      namespace manager
      {
         namespace admin
         {
            namespace vo
            {
               inline namespace v1
               {
                  using id_type = common::platform::size::type;
                  using size_type = common::platform::size::type;

                  struct Group
                  {
                     id_type id;
                     std::string name;
                     std::string note;

                     std::vector< id_type> dependencies;
                     std::vector< std::string> resources;

                     CASUAL_CONST_CORRECT_SERIALIZE({
                        archive & CASUAL_MAKE_NVP( id);
                        archive & CASUAL_MAKE_NVP( name);
                        archive & CASUAL_MAKE_NVP( note);
                        archive & CASUAL_MAKE_NVP( resources);
                        archive & CASUAL_MAKE_NVP( dependencies);
                     })

                  };

                  struct Process
                  {
                     using pid_type = common::strong::process::id;

                     id_type id;
                     std::string alias;
                     std::string path;
                     std::vector< std::string> arguments;
                     std::string note;

                     std::vector< id_type> memberships;

                     struct
                     {
                        std::vector< std::string> variables;

                        CASUAL_CONST_CORRECT_SERIALIZE({
                           archive & CASUAL_MAKE_NVP( variables);
                        })

                     } environment;

                     bool restart = false;
                     size_type restarts = 0;

                     CASUAL_CONST_CORRECT_SERIALIZE({
                        archive & CASUAL_MAKE_NVP( id);
                        archive & CASUAL_MAKE_NVP( alias);
                        archive & CASUAL_MAKE_NVP( path);
                        archive & CASUAL_MAKE_NVP( arguments);
                        archive & CASUAL_MAKE_NVP( note);
                        archive & CASUAL_MAKE_NVP( memberships);
                        archive & CASUAL_MAKE_NVP( environment);
                        archive & CASUAL_MAKE_NVP( restart);
                        archive & CASUAL_MAKE_NVP( restarts);
                     })

                     inline friend bool operator < ( const Process& lhs, const Process& rhs) { return lhs.id < rhs.id;}

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
                  } // instance

                  template< typename H>
                  struct Instance
                  {
                     H handle;
                     instance::State state = instance::State::scale_out;

                     friend bool operator == ( const Instance& lhs, const H& rhs) { return common::process::id( lhs.handle) == common::process::id( rhs);}

                     CASUAL_CONST_CORRECT_SERIALIZE({
                        archive & CASUAL_MAKE_NVP( handle);
                        archive & CASUAL_MAKE_NVP( state);
                     })
                  };

                  struct Executable : Process
                  {
                     using instance_type = Instance< common::strong::process::id>;
                     std::vector< instance_type> instances;

                     CASUAL_CONST_CORRECT_SERIALIZE({
                        Process::serialize( archive);
                        archive & CASUAL_MAKE_NVP( instances);
                     })
                  };

                  struct Server : Process
                  {
                     using instance_type = Instance< common::process::Handle>;
                     std::vector< instance_type> instances;

                     std::vector< std::string> resources;
                     std::vector< std::string> restriction;

                     CASUAL_CONST_CORRECT_SERIALIZE({
                        Process::serialize( archive);
                        archive & CASUAL_MAKE_NVP( instances);
                        archive & CASUAL_MAKE_NVP( resources);
                        archive & CASUAL_MAKE_NVP( restriction);
                     })
                  };


                  struct State
                  {
                     std::vector< vo::Group> groups;
                     std::vector< vo::Executable> executables;
                     std::vector< vo::Server> servers;

                     struct
                     {
                        std::vector< common::process::Handle> listeners;

                        CASUAL_CONST_CORRECT_SERIALIZE(
                        {
                           archive & CASUAL_MAKE_NVP( listeners);
                        })

                     } event;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        archive & CASUAL_MAKE_NVP( groups);
                        archive & CASUAL_MAKE_NVP( executables);
                        archive & CASUAL_MAKE_NVP( servers);
                        archive & CASUAL_MAKE_NVP( event);
                     })

                  };


                  namespace scale
                  {
                     struct Instances
                     {
                        std::string alias;
                        size_type instances;

                        template< typename A>
                        void serialize( A& archive)
                        {
                           archive & CASUAL_MAKE_NVP( alias);
                           archive & CASUAL_MAKE_NVP( instances);
                        }
                     };
                  } // scale

               } // v1_0
            } // vo
         } // admin
      } // manager
   } // domain
} // casual

#endif // BROKERVO_H_
