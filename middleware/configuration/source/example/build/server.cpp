//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "configuration/example/build/server.h"
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
            namespace server
            {

               configuration::build::server::Server example()
               {

                  static constexpr auto yaml = R"(
server:
  default:
    service:
      transaction: join
      category: some.category


  resources:
    - key: rm-mockup
      name: resource-1
      note: the runtime configuration for this resource is correlated with the name 'resource-1' - no group is needed for resource configuration

  services:
    - name: s1

    - name: s2
      transaction: auto

    - name: s3
      function: f3

    - name: s4
      function: f4
      category: some.other.category
)";

                  return create::model< configuration::build::server::Server>( yaml, "server");
               }

            } // server
         } // build
      } // example
   } // configuration
} // casual
