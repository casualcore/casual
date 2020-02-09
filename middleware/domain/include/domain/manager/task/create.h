//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "domain/manager/task.h"
#include "domain/manager/state.h"
#include "domain/manager/admin/model.h"

namespace casual
{
   namespace domain
   {
      namespace manager
      {
         namespace task
         {
            namespace create
            {
               namespace restart
               {
                  manager::Task server( State& state, state::Server::id_type id);
                  manager::Task executable( State& state, state::Executable::id_type id);
               } // restart

               namespace scale
               {
                  manager::Task server( const State& state, state::Server::id_type id);
                  manager::Task executable( const State& state, state::Executable::id_type id);

                  std::vector< manager::Task> dependency( const State& state, std::vector< state::dependency::Group> groups);
               } // scale


               //! groups tasks into a sequential task with the most restrictive Completion of all tasks 
               manager::Task group( std::string description, std::vector< manager::Task>&& tasks);
;

               namespace done
               {  
                  // sentinel when the boot (all batches) is done
                  manager::Task boot();

                  // sentinel when the shutdown (all batches) is done
                  manager::Task shutdown();
                  
               } // done


               //! "trigger" to shutdown the domain
               manager::Task shutdown();

            } // create
         } // task
      } // manager
   } // domain
} // casual