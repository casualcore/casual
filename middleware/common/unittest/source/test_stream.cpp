//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <common/unittest.h>

#include "common/stream.h"

#include "common/chronology.h"
#include "common/strong/id.h"

namespace casual
{

   namespace common
   {
      TEST( common_stream, resource_id)
      {
         unittest::Trace trace;
         auto id = strong::resource::id{ -1};

         std::ostringstream out;
         stream::write( out, id);
         EXPECT_TRUE( out.str() == "E-1") << "out.str(): " << out.str();

      }

      TEST( common_stream, duration_ns)
      {
         unittest::Trace trace;

         std::ostringstream out;
         stream::write( out, std::chrono::nanoseconds( 560));
         EXPECT_TRUE( out.str() == "560ns") << "out.str(): " << out.str();

      }

   } // common
   
} // casual