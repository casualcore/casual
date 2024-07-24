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
         state::resource::proxy::Instance instance{ {}, {}};
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
         state::resource::proxy::Instance instance{ {}, {}};
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


         auto pending = instance.pending();
         EXPECT_TRUE( pending.count == 0);
         EXPECT_TRUE( pending.total == platform::time::unit{});
      }

      TEST( transaction_manager_state, proxy_instance_pending)
      {
         state::resource::proxy::Instance instance{ {}, {}};
         instance.state( decltype( instance.state())::idle);

         static constexpr auto duration = std::chrono::milliseconds{ 1};

         // will get reservation time = now
         instance.reserve( platform::time::clock::type::now() -  duration);
         instance.unreserve( local::resource_roundtrip( duration));

         // make sure metrics woks as before
         { 
            auto metrics = instance.metrics();

            EXPECT_TRUE( metrics.resource.count == 1);
            EXPECT_TRUE( metrics.resource.limit.max > duration);
            EXPECT_TRUE( metrics.resource.limit.min > duration);

            EXPECT_TRUE( metrics.roundtrip.count == 1);
            EXPECT_TRUE( metrics.roundtrip.limit.min > metrics.resource.limit.min) << CASUAL_NAMED_VALUE( metrics);
            EXPECT_TRUE( metrics.roundtrip.limit.max > metrics.resource.limit.max);

            EXPECT_TRUE( metrics.roundtrip.total > metrics.resource.total);
         }

         {
            auto pending = instance.pending();

            // we need to compensate for the time it takes for `instance` to take current time internally
            // 0.5ms seems more than enough.
            auto check_span = []( auto value, auto duration)
            {
               return value >= duration && value < duration + std::chrono::microseconds{ 500};
            };

            EXPECT_TRUE( pending.count == 1);
            EXPECT_TRUE( check_span( pending.total, duration));
            EXPECT_TRUE( check_span( pending.limit.min, duration));
            EXPECT_TRUE( check_span( pending.limit.max, duration));
         }
      }

      TEST( transaction_manager_state, proxy_instance_multiple_pending)
      {
         state::resource::proxy::Instance instance{ {}, {}};
         instance.state( decltype( instance.state())::idle);

         auto pending_roundtrip = [ &instance]( auto requested)
         {
            instance.reserve( requested);
            // we don't verify the roundtrip -> we can make it real small.
            instance.unreserve( local::resource_roundtrip( std::chrono::microseconds{ 5}));
         };

         static constexpr auto pending_durations = common::array::make( 
            std::chrono::milliseconds{ 1000}, 
            std::chrono::milliseconds{ 500}, 
            std::chrono::milliseconds{ 250}, 
            std::chrono::milliseconds{ 100}, 
            std::chrono::milliseconds{ 10});

         for( auto& duration : pending_durations)
            pending_roundtrip( platform::time::clock::type::now() - duration);

         auto total = algorithm::accumulate( pending_durations, std::chrono::microseconds{});

         const auto min = *algorithm::min( pending_durations);
         const auto max = *algorithm::max( pending_durations);

         {
            // we need to compensate for the time it takes for `instance` to take current time internally
            // 5ms seems more than enough.
            auto check_span = []( auto value, auto duration)
            {
               return value >= duration && value < duration + std::chrono::milliseconds{ 5};
            };

            auto pending = instance.pending();

            EXPECT_TRUE( pending.count == range::size( pending_durations));
            EXPECT_TRUE( check_span( pending.total, total)) << CASUAL_NAMED_VALUE( pending.total) << "\n        " << CASUAL_NAMED_VALUE( total);
            EXPECT_TRUE( check_span( pending.limit.min, min));
            EXPECT_TRUE( check_span( pending.limit.max, max));
         }
      }
      
   } // transaction::manager
   
} // casual