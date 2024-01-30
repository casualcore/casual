//!
//! Copyright (c) 2023, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"


#include "gateway/group/inbound/state.h"

namespace casual
{
   using namespace common;

   namespace gateway::group::inbound
   {
      TEST( gateway_inbound_state, transaction_cache_associate)
      {
         auto trid = transaction::id::create();
         auto descriptor = strong::socket::id{ 10};

         state::transaction::Cache cache;
         EXPECT_TRUE( cache.empty());

         EXPECT_TRUE( cache.associate( descriptor, trid) == nullptr);
         EXPECT_TRUE( ! cache.empty());

         auto extracted = cache.extract( descriptor);
         EXPECT_EQ( std::ssize( extracted), 1);
         EXPECT_TRUE( algorithm::find( extracted, trid));

         EXPECT_TRUE( cache.empty());
      }

      TEST( gateway_inbound_state, transaction_cache_dissociate)
      {
         auto trid = transaction::id::create();
         auto descriptor = strong::socket::id{ 10};

         state::transaction::Cache cache;
         EXPECT_TRUE( cache.associate( descriptor, trid) == nullptr);

         cache.dissociate( descriptor, trid);

         EXPECT_TRUE( cache.empty());
      }

      TEST( gateway_inbound_state, transaction_cache_add_branch)
      {
         auto trid = transaction::id::create();
         auto branch = transaction::id::branch( trid);
         auto descriptor = strong::socket::id{ 10};

         state::transaction::Cache cache;
         EXPECT_TRUE( cache.associate( descriptor, trid) == nullptr);

         cache.add_branch( descriptor, branch);

         {
            auto found = cache.find( transaction::id::range::global( trid));
            ASSERT_TRUE( found);
            EXPECT_TRUE( *found == branch);
         }

         cache.dissociate( descriptor, trid);

         EXPECT_TRUE( cache.empty());
      }

      TEST( gateway_inbound_state, transaction_cache_add_branch_extract)
      {
         auto trid = transaction::id::create();
         auto branch = transaction::id::branch( trid);
         auto descriptor = strong::socket::id{ 10};

         state::transaction::Cache cache;
         EXPECT_TRUE( cache.associate( descriptor, trid) == nullptr);

         cache.add_branch( descriptor, branch);
         
         auto extracted = cache.extract( descriptor);
         EXPECT_EQ( std::ssize( extracted), 1);
         EXPECT_TRUE( algorithm::find( extracted, trid));

         EXPECT_TRUE( cache.empty());
      }

      TEST( gateway_inbound_state, transaction_cache_10_associations)
      {
         // represent a "call"
         struct Call
         {
            transaction::ID trid;
            std::array< strong::socket::id, 2> descriptors;

            // so we can check it
            transaction::ID branched;
         };

         std::array< Call, 10> calls;

         std::ranges::generate( calls, []()
         {
            static int current = 10; 
            Call result;
            result.trid = transaction::id::create();
            result.descriptors[ 0] = strong::socket::id{ current++};
            result.descriptors[ 1] = strong::socket::id{ current++};
            return result;
         });

         state::transaction::Cache cache;

         // associate first descriptor
         algorithm::for_each( calls, [ &cache]( auto& call)
         {
            cache.associate( call.descriptors[ 0], call.trid);
         });
         
         algorithm::for_each( calls, [ &cache]( auto& call)
         {
            call.branched = transaction::id::branch( call.trid);
            cache.add_branch( call.descriptors[ 0], call.branched );
         });

         // check that only first descriptor is associated
         algorithm::for_each( calls, [ &cache]( auto& call)
         {
            EXPECT_TRUE( cache.associated( call.descriptors[ 0]));
            EXPECT_TRUE( ! cache.associated( call.descriptors[ 1]));
         });
         
         // associate second descriptor
         algorithm::for_each( calls, [ &cache]( auto& call)
         {
            cache.associate( call.descriptors[ 1], call.trid);
         });

         // check that both descriptor is associated
         algorithm::for_each( calls, [ &cache]( auto& call)
         {
            EXPECT_TRUE( cache.associated( call.descriptors[ 0]));
            EXPECT_TRUE( cache.associated( call.descriptors[ 1]));
         });

         // check that both descriptor is associated
         algorithm::for_each( calls, [ &cache]( auto& call)
         {
            auto trid = cache.find( transaction::id::range::global( call.trid));
            ASSERT_TRUE( trid);
            EXPECT_TRUE( *trid == call.branched);
         });

         // dissociate first descriptor, check that second still associated and branched still in cache
         algorithm::for_each( calls, [ &cache]( auto& call)
         {
            cache.dissociate( call.descriptors[ 0], call.trid);
            EXPECT_TRUE( ! cache.associated( call.descriptors[ 0]));
            EXPECT_TRUE( cache.associated( call.descriptors[ 1]));
            EXPECT_TRUE( cache.find( transaction::id::range::global( call.trid)));
         });

         // dissociate second descriptor, check that branched is removed from cache
         algorithm::for_each( calls, [ &cache]( auto& call)
         {
            cache.dissociate( call.descriptors[ 1], call.trid);
            EXPECT_TRUE( ! cache.associated( call.descriptors[ 1]));
            EXPECT_TRUE( ! cache.find( transaction::id::range::global( call.trid)));
         });

         EXPECT_TRUE( cache.empty());
      }

   } // gateway::group::inbound
   
} // casual