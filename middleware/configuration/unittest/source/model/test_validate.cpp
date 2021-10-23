//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"
#include "common/unittest/file.h"

#include "configuration/model.h"
#include "configuration/model/load.h"
#include "configuration/model/validate.h"

#include "common/code/casual.h"

#include "common/serialize/create.h"

namespace casual
{
   using namespace common;

   namespace configuration::model
   {
      namespace local
      {
         namespace
         {
            auto configuration = []( auto content)
            {
               auto path = unittest::file::temporary::content( ".yaml", std::move( content));
               return configuration::model::load( { path});
            };
         } // <unnamed>
      } // local

      TEST( configuration_model_validate, domain_expect_throw_invalid_configuration)
      {
         constexpr auto configuration = R"(
domain:
  name: model
  servers:
    - path: a
      instances: -1

)";

         EXPECT_THROW( local::configuration( configuration), std::system_error);
         
      }

      TEST( configuration_model_validate, domain_expect_no_throw)
      {
         constexpr auto configuration = R"(
domain:
  name: model
  servers:
    - path: a
      instances: 0

)";

         EXPECT_NO_THROW( local::configuration( configuration));
         
      }
      
   } // configuration::model
} // casual