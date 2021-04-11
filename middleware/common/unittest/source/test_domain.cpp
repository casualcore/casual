//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"
#include "common/unittest/file.h"
#include "common/environment.h"
#include "common/domain.h"
#include "common/code/casual.h"


namespace casual
{
   namespace common
   {

      TEST( common_domain, create_domain_lock_file)
      {
         unittest::Trace trace;

         unittest::directory::temporary::Scoped domain_home;
         environment::variable::set( "CASUAL_DOMAIN_HOME", domain_home);
         environment::reset();


         EXPECT_NO_THROW(
            domain::singleton::create( process::handle(), domain::identity());
         );
      }

      TEST( common_domain, create_2_domain_lock_files__expect_throw)
      {
         unittest::Trace trace;

         unittest::directory::temporary::Scoped domain_home;
         environment::variable::set( "CASUAL_DOMAIN_HOME", domain_home);
         environment::reset();

         EXPECT_CODE(
            auto path = domain::singleton::create( process::handle(), domain::identity());

            domain::singleton::create( process::handle(), domain::identity());

         , code::casual::domain_running);
      }


   } // common

} // casual
