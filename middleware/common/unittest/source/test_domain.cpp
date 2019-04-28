//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"
#include "common/domain.h"
#include "common/exception/system.h"


namespace casual
{
   namespace common
   {

      TEST( common_domain, create_domain_lock_file)
      {
         unittest::Trace trace;

         EXPECT_NO_THROW(
               domain::singleton::create( process::handle(), domain::identity());
         );
      }

      TEST( common_domain, create_2_domain_lock_files__expect_throw)
      {
         unittest::Trace trace;

         EXPECT_THROW(
               auto path = domain::singleton::create( process::handle(), domain::identity());

               domain::singleton::create( process::handle(), domain::identity());

         , exception::system::invalid::Process);
      }


   } // common

} // casual
