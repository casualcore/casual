//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "domain/manager/task.h"
#include "domain/manager/state.h"

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

               namespace batch
               {
                  manager::Task boot( state::Batch batch);
                  manager::Task shutdown( state::Batch batch);


               } // batch

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