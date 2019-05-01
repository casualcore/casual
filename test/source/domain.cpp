//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "casual/test/domain.h"
#include "common/environment.h"

namespace casual
{
   namespace test
   {
      namespace domain
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

         Manager::Manager( const std::vector< std::string>& configuration)
            : casual::domain::manager::unittest::Process{ configuration, local::environment()}
         {

         }
         

      } // domain
   } // test
} // casual