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
               inline namespace v1_0
               {
                  using id_type = std::size_t;

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

                  struct Executable
                  {
                     using pid_type = common::platform::pid::type;

                     id_type id;
                     std::string alias;
                     std::string path;
                     std::vector< std::string> arguments;
                     std::string note;

                     std::vector< pid_type> instances;
                     std::vector< id_type> memberships;

                     struct
                     {
                        std::vector< std::string> variables;

                        CASUAL_CONST_CORRECT_SERIALIZE({
                           archive & CASUAL_MAKE_NVP( variables);
                        })

                     } environment;


                     std::size_t configured_instances = 0;

                     bool restart = false;
                     std::size_t restarts = 0;


                     CASUAL_CONST_CORRECT_SERIALIZE({
                        archive & CASUAL_MAKE_NVP( id);
                        archive & CASUAL_MAKE_NVP( alias);
                        archive & CASUAL_MAKE_NVP( path);
                        archive & CASUAL_MAKE_NVP( arguments);
                        archive & CASUAL_MAKE_NVP( note);
                        archive & CASUAL_MAKE_NVP( instances);
                        archive & CASUAL_MAKE_NVP( memberships);
                        archive & CASUAL_MAKE_NVP( environment);
                        archive & CASUAL_MAKE_NVP( configured_instances);
                        archive & CASUAL_MAKE_NVP( restart);
                        archive & CASUAL_MAKE_NVP( restarts);
                     })

                  };


                  struct State
                  {
                     std::vector< vo::Group> groups;
                     std::vector< vo::Executable> executables;

                     struct
                     {
                        std::vector< common::process::Handle> listeners;

                        CASUAL_CONST_CORRECT_SERIALIZE(
                        {
                           archive & CASUAL_MAKE_NVP( listeners);
                        })

                     } termination;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        archive & CASUAL_MAKE_NVP( groups);
                        archive & CASUAL_MAKE_NVP( executables);
                        archive & CASUAL_MAKE_NVP( termination);
                     })

                  };


                  namespace scale
                  {
                     struct Instances
                     {
                        std::string alias;
                        std::size_t instances;

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
