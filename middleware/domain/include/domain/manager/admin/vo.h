//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "common/serialize/macro.h"
#include "common/platform.h"

#include "configuration/environment.h"

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
                        CASUAL_SERIALIZE( id);
                        CASUAL_SERIALIZE( name);
                        CASUAL_SERIALIZE( note);
                        CASUAL_SERIALIZE( resources);
                        CASUAL_SERIALIZE( dependencies);
                     })

                  };

                  struct Process
                  {
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
                           CASUAL_SERIALIZE( variables);
                        })

                     } environment;

                     bool restart = false;
                     size_type restarts = 0;

                     CASUAL_CONST_CORRECT_SERIALIZE({
                        CASUAL_SERIALIZE( id);
                        CASUAL_SERIALIZE( alias);
                        CASUAL_SERIALIZE( path);
                        CASUAL_SERIALIZE( arguments);
                        CASUAL_SERIALIZE( note);
                        CASUAL_SERIALIZE( memberships);
                        CASUAL_SERIALIZE( environment);
                        CASUAL_SERIALIZE( restart);
                        CASUAL_SERIALIZE( restarts);
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
                     common::platform::time::point::type spawnpoint;

                     friend bool operator == ( const Instance& lhs, const H& rhs) { return common::process::id( lhs.handle) == common::process::id( rhs);}

                     CASUAL_CONST_CORRECT_SERIALIZE({
                        CASUAL_SERIALIZE( handle);
                        CASUAL_SERIALIZE( state);
                        CASUAL_SERIALIZE( spawnpoint);
                     })
                  };

                  struct Executable : Process
                  {
                     using instance_type = Instance< common::strong::process::id>;
                     std::vector< instance_type> instances;

                     CASUAL_CONST_CORRECT_SERIALIZE({
                        Process::serialize( archive);
                        CASUAL_SERIALIZE( instances);
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
                        CASUAL_SERIALIZE( instances);
                        CASUAL_SERIALIZE( resources);
                        CASUAL_SERIALIZE( restriction);
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
                           CASUAL_SERIALIZE( listeners);
                        })

                     } event;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        CASUAL_SERIALIZE( groups);
                        CASUAL_SERIALIZE( executables);
                        CASUAL_SERIALIZE( servers);
                        CASUAL_SERIALIZE( event);
                     })

                  };


                  namespace scale
                  {
                     struct Instances
                     {
                        std::string alias;
                        size_type instances;

                        CASUAL_CONST_CORRECT_SERIALIZE
                        (
                           CASUAL_SERIALIZE( alias);
                           CASUAL_SERIALIZE( instances);
                        )
                     };
                  } // scale

                  namespace restart
                  {
                     struct Instances
                     {
                        std::string alias;

                        CASUAL_CONST_CORRECT_SERIALIZE({
                           CASUAL_SERIALIZE( alias);
                        })
                     };
                     
                     struct Result
                     {
                        std::string alias;
                        common::strong::task::id task;
                        std::vector< common::strong::process::id> pids;

                        CASUAL_CONST_CORRECT_SERIALIZE({
                           CASUAL_SERIALIZE( alias);
                           CASUAL_SERIALIZE( task);
                           CASUAL_SERIALIZE( pids);
                        })
                     };
                  
                  } // restart

                  namespace set
                  {
                     struct Environment
                     {
                        casual::configuration::Environment variables;
                        std::vector< std::string> aliases;

                        CASUAL_CONST_CORRECT_SERIALIZE
                        (
                           CASUAL_SERIALIZE( variables);
                           CASUAL_SERIALIZE( aliases);
                        )
                     };
                     
                  } // set

               } // v1_0
            } // vo
         } // admin
      } // manager
   } // domain
} // casual


