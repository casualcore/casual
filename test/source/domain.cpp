//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "casual/test/domain.h"
#include "common/environment.h"

namespace casual
{
   namespace test::domain
   {
      namespace local
      {
         namespace
         {
            auto environment()
            {
               return []( const std::string& home)
               {
                  // create directores for nginx...
                  common::directory::create( home + "/logs");
               };
            }
            
         } // <unnamed>
      } // local

      Manager::Manager( std::vector< std::string_view> configuration)
         : casual::domain::manager::unittest::Process{ configuration, local::environment()}
      {

      }
         
   } // test::domain
} // casual