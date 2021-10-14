//! 
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "configuration/model.h"

#include "common/code/raise.h"
#include "common/code/casual.h"
#include "common/algorithm.h"

namespace casual
{
   using namespace common;

   namespace configuration
   {
      namespace model
      {
         namespace local
         {
            namespace
            {
               namespace validate
               {
                  auto instances = []( auto& value)
                  {
                     if( value.instances < 0)
                        code::raise::error( code::casual::invalid_configuration, "number of instances cannot be negative");
                  };
               } // validate
            } // <unnamed>
         } // local

         void validate( Model& model)
         {
            // domain
            algorithm::for_each( model.domain.servers, local::validate::instances);
            algorithm::for_each( model.domain.executables, local::validate::instances);

            // transaction
            algorithm::for_each( model.transaction.resources, local::validate::instances);

            // queue
            algorithm::for_each( model.queue.forward.groups, []( auto& group)
            {
               algorithm::for_each( group.services, local::validate::instances);
            });
         }
      } // model
      
   } // configuration

} // casual


