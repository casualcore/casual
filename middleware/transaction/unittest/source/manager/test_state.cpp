//!
//! Copyright (c) 2023, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "transaction/manager/state.h"

namespace casual
{
   using namespace common;

   namespace transaction::manager
   {
      namespace local
      {
         namespace
         {
            auto resource_roundtrip( platform::time::unit duration)
            {
               // transport tm -> rm
               process::sleep( duration);

               common::message::Statistics time;

               time.start = platform::time::clock::type::now();
               // xa time
               process::sleep( duration);
               time.end = platform::time::clock::type::now();

               // transport rm -> tm
               process::sleep( duration);

               return time;
            }
         } // <unnamed>
      } // local

      TEST( transaction_manager_state, proxy_instance_metric)
      {
         state::resource::Proxy::Instance instance;
         instance.state( decltype( instance.state())::idle);

         static constexpr auto duration = std::chrono::microseconds{ 100};

         // will get reservation time = now
         instance.reserve();
         instance.unreserve( local::resource_roundtrip( duration));

         auto metrics = instance.metrics();

         EXPECT_TRUE( metrics.resource.count == 1);
         EXPECT_TRUE( metrics.resource.limit.max > duration);
         EXPECT_TRUE( metrics.resource.limit.min > duration);

         EXPECT_TRUE( metrics.roundtrip.count == 1);
         EXPECT_TRUE( metrics.roundtrip.limit.min > metrics.resource.limit.min) << CASUAL_NAMED_VALUE( metrics);
         EXPECT_TRUE( metrics.roundtrip.limit.max > metrics.resource.limit.max);

         EXPECT_TRUE( metrics.roundtrip.total > metrics.resource.total);
      }

      TEST( transaction_manager_state, proxy_instance_metric_100)
      {
         state::resource::Proxy::Instance instance;
         instance.state( decltype( instance.state())::idle);

         static constexpr auto duration = std::chrono::microseconds{ 10};

         algorithm::for_n< 100>( [ &instance]()
         {
            instance.reserve();
            instance.unreserve( local::resource_roundtrip( duration));
         });


         auto metrics = instance.metrics();

         EXPECT_TRUE( metrics.resource.count == 100);
         EXPECT_TRUE( metrics.resource.limit.max > duration);
         EXPECT_TRUE( metrics.resource.limit.min > duration);

         EXPECT_TRUE( metrics.roundtrip.count == 100);
         EXPECT_TRUE( metrics.roundtrip.limit.min > metrics.resource.limit.min) << CASUAL_NAMED_VALUE( metrics);
         EXPECT_TRUE( metrics.roundtrip.limit.max > metrics.resource.limit.max);

         EXPECT_TRUE( metrics.roundtrip.total > metrics.resource.total);
      }
      
   } // transaction::manager
   
} // casual