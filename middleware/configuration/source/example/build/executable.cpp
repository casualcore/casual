//! 
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "configuration/example/build/executable.h"
#include "configuration/example/create.h"

#include "common/serialize/create.h"

#include <fstream>

namespace casual
{
   namespace configuration
   {
      namespace example
      {
         namespace build
         {
            namespace executable
            {

               configuration::build::Executable example()
               {

                  static constexpr auto yaml = R"(
executable:
  resources:
    - key: rm-mockup
      name: resource-1
      note: the runtime configuration for this resource is correlated with the name 'resource-1' - no group is needed for resource configuration


  entrypoint: start
)";

                  return create::model< configuration::build::Executable>( yaml, "executable");
               }

            } // executable
         } // build
      } // example
   } // configuration
} // casual
