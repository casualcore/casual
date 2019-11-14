//! 
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/platform.h"

#include <string>
#include <vector>

namespace casual
{
   namespace domain
   {
      namespace manager
      {
         namespace api
         {
            inline namespace v1 {

            namespace model
            {
               namespace uuid
               {
                  using type = casual::platform::uuid::type;
               } // uuid

               namespace process
               {
                  namespace id
                  {
                     using type = casual::platform::process::native::type;
                  } // id
                  
                  struct Handle 
                  {
                     id::type pid;
                     uuid::type ipc;
                  };
               } // process


               using id_type = casual::platform::size::type;
               using size_type = casual::platform::size::type;

               struct Group
               {
                  id_type id;
                  std::string name;
                  std::string note;

                  std::vector< id_type> dependencies;
                  std::vector< std::string> resources;
               };

               struct base_process
               {
                  //! @note not pid, but the internal id of the servcer/executable
                  id_type id;
                  std::string alias;
                  std::string path;
                  std::vector< std::string> arguments;
                  std::string note;

                  std::vector< id_type> memberships;

                  struct
                  {
                     std::vector< std::string> variables;
                  } environment;

                  bool restart = false;
                  size_type restarts = 0;
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
               struct basic_instance
               {
                  H handle;
                  instance::State state = instance::State::scale_out;
                  casual::platform::time::point::type spawnpoint;
               };

               struct Executable : base_process
               {
                  using instance_type = basic_instance< process::id::type>;
                  std::vector< instance_type> instances;
               };

               struct Server : base_process
               {
                  using instance_type = basic_instance< process::Handle>;
                  std::vector< instance_type> instances;

                  std::vector< std::string> resources;
                  std::vector< std::string> restriction;
               };

            } // model

            struct Model
            {
               std::vector< model::Group> groups;
               std::vector< model::Executable> executables;
               std::vector< model::Server> servers;
            };


            } // v1
         } // api
      } // manager
   } // domain
} // casual
