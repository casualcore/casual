//!
//! casual 
//!

#include <common/unittest.h>

#include "gateway/inbound/cache.h"


namespace casual
{
   using namespace common;
   namespace gateway
   {

      namespace local
      {
         namespace
         {
            platform::binary::type payload()
            {
               return { '1', '2', '3', 4};
            }
         } // <unnamed>
      } // local

      TEST( casual_gateway_inbound_cache, instantiation)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW({
            inbound::Cache cache;
         });
      }

      TEST( casual_gateway_inbound_cache, add_get__expect_same)
      {
         common::unittest::Trace trace;

         inbound::Cache cache;

         auto correlation = uuid::make();
         {
            communication::message::Complete complete{ message::Type::gateway_ipc_connect_request, correlation};
            complete.payload = local::payload();
            cache.add( std::move( complete));
         }
         auto complete = cache.get( correlation);

         EXPECT_TRUE( complete.correlation == correlation);
         EXPECT_TRUE( complete.type == message::Type::gateway_ipc_connect_request);
         EXPECT_TRUE( complete.payload == local::payload());
      }

      namespace local
      {
         namespace
         {
            void producer( const inbound::Cache& cache, const Uuid& correlation, std::size_t count)
            {
               try
               {
                  while( count-- > 0)
                  {
                     communication::message::Complete complete{ message::Type::gateway_ipc_connect_request, correlation};
                     complete.payload = local::payload();
                     cache.add( std::move( complete));
                  }
               }
               catch( ...)
               {
                  common::exception::handle();
               }
            }
         } // <unnamed>
      } // local

      TEST( casual_gateway_inbound_cache, add_get__from_other_thread__10_times__expect_same)
      {
         common::unittest::Trace trace;

         inbound::Cache cache;

         auto correlation = uuid::make();

         std::thread worker{ &local::producer, std::ref( cache), correlation, 10};


         auto count = 10;

         while( count > 0)
         {
            if( cache.size().messages > 0)
            {
               --count;
               auto complete = cache.get( correlation);

               EXPECT_TRUE( complete.correlation == correlation);
               EXPECT_TRUE( complete.type == message::Type::gateway_ipc_connect_request);
               EXPECT_TRUE( complete.payload == local::payload());
            }
         }
         worker.join();
      }

      TEST( casual_gateway_inbound_cache, add_get__limit_10_messages__from_other_thread__100_times__expect_at_most_10_messages_in_cache)
      {
         common::unittest::Trace trace;

         inbound::Cache cache{ inbound::Cache::Limit{ 0, 10}};

         auto correlation = uuid::make();

         auto count = 100;
         std::thread worker{ &local::producer, std::ref( cache), correlation, count};

         while( cache.size().messages < 10)
         {
            process::sleep( std::chrono::milliseconds{ 1});
         }

         while( count > 0)
         {
            if( cache.size().messages > 0)
            {
               --count;
               EXPECT_TRUE( cache.size().messages <= 10);

               auto complete = cache.get( correlation);
               EXPECT_TRUE( complete.correlation == correlation);
               EXPECT_TRUE( complete.type == message::Type::gateway_ipc_connect_request);
               EXPECT_TRUE( complete.payload == local::payload());
            }
         }
         worker.join();
      }

   } // gateway
} // casual
