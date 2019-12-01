//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <gtest/gtest.h>
#include "common/unittest.h"


#include "transaction/manager/admin/model.h"
#include "transaction/manager/admin/transform.h"

#include "transaction/manager/state.h"


#include "serviceframework/log.h"

namespace casual
{
   namespace transaction
   {
      namespace manager
      {

         
         TEST( casual_transaction_admin, transform_metrics)
         {
            common::unittest::Trace trace;

            state::Metrics metrics;
            {
               metrics.resource += std::chrono::milliseconds{ 42};
               metrics.resource += std::chrono::seconds{ 42};

               metrics.roundtrip += std::chrono::milliseconds{ 142};
               metrics.roundtrip += std::chrono::seconds{ 142};
            }

            // transform to admin and back.
            auto result = admin::transform::metrics( admin::transform::metrics( metrics));

            EXPECT_TRUE( metrics.roundtrip == result.roundtrip);
            EXPECT_TRUE( metrics.resource == result.resource);

         }
         
      } // manager
   } // transaction

} // casual

