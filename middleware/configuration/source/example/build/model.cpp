//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "configuration/example/build/model.h"

#include "configuration/example/create.h"

namespace casual
{
   namespace configuration::example::build::model
   {
      configuration::build::server::Model server()
      {

         return create::model< configuration::build::server::Model>( R"(
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
)");

                  
      }

      configuration::build::executable::Model executable()
      {
         return create::model< configuration::build::executable::Model>( R"(
executable:
  resources:
    - key: rm-mockup
      name: resource-1
      note: the runtime configuration for this resource is correlated with the name 'resource-1' - no group is needed for resource configuration

  entrypoint: start
)");


      }

   } // configuration::example::build::model
} // casual