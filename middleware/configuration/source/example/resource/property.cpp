//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "configuration/example/resource/property.h"
#include "configuration/example/create.h"

#include "common/serialize/create.h"

#include <fstream>

namespace casual
{
   namespace configuration
   {
      namespace example
      {
         namespace resource
         {
            namespace property
            {
               std::vector< configuration::resource::Property> example()
               {

                  static constexpr auto yaml = R"(
resources:
  - key: db2
    server: rm-proxy-db2-static
    xa_struct_name: db2xa_switch_static_std
    libraries:
      - db2

    paths:
      library:
        - ${DB2DIR}/lib64

  - key: rm-mockup
    server: rm-proxy-casual-mockup
    xa_struct_name: casual_mockup_xa_switch_static
    libraries:
      - casual-mockup-rm
    paths:
      library:
        - ${CASUAL_HOME}/lib
)";

                  return create::model< std::vector< configuration::resource::Property>>( yaml, "resources");
               }

            } // property

         } // resource

      } // example
   } // configuration
} // casual
