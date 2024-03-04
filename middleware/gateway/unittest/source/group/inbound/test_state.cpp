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

         EXPECT_TRUE( cache.associate( trid, descriptor) == nullptr);

         // still empty
         EXPECT_TRUE( cache.empty());

         auto extracted = cache.failed( descriptor);
         EXPECT_TRUE( std::empty( extracted));
      }

      TEST( gateway_inbound_state, transaction_cache_dissociate)
      {
         auto trid = transaction::id::create();
         auto descriptor = strong::socket::id{ 10};

         state::transaction::Cache cache;
         EXPECT_TRUE( cache.associate( trid, descriptor) == nullptr);

         cache.remove( transaction::id::range::global( trid));

         EXPECT_TRUE( cache.empty());
      }

      TEST( gateway_inbound_state, transaction_cache_add_branch)
      {
         auto trid = transaction::id::create();
         auto branch = transaction::id::branch( trid);
         auto descriptor = strong::socket::id{ 10};

         state::transaction::Cache cache;
         EXPECT_TRUE( cache.associate( trid, descriptor) == nullptr);

         cache.add( branch, descriptor);

         {
            auto found = cache.find( transaction::id::range::global( trid));
            ASSERT_TRUE( found);
            EXPECT_TRUE( *found == branch);
         }

         cache.remove( transaction::id::range::global( trid));

         EXPECT_TRUE( cache.empty());
      }

      TEST( gateway_inbound_state, transaction_cache_add_branch_extract)
      {
         auto trid = transaction::id::create();
         auto branch = transaction::id::branch( trid);
         auto descriptor = strong::socket::id{ 10};

         state::transaction::Cache cache;
         EXPECT_TRUE( cache.associate( trid, descriptor) == nullptr);

         cache.add( branch, descriptor);
         
         auto extracted = cache.failed( descriptor);
         EXPECT_EQ( std::ssize( extracted), 1);
         EXPECT_TRUE( algorithm::find( extracted, branch));

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
            cache.associate( call.trid, call.descriptors[ 0]);
         });
         
         algorithm::for_each( calls, [ &cache]( auto& call)
         {
            call.branched = transaction::id::branch( call.trid);
            cache.add( call.branched, call.descriptors[ 0]);
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
            cache.associate( call.trid, call.descriptors[ 1]);
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
            EXPECT_TRUE( cache.associated( call.descriptors[ 0]));
            EXPECT_TRUE( cache.associated( call.descriptors[ 1]));
         });

         // remove the trid
         algorithm::for_each( calls, [ &cache]( auto& call)
         {
            cache.remove( transaction::id::range::global( call.trid));
            EXPECT_TRUE( ! cache.associated( call.descriptors[ 0]));
            EXPECT_TRUE( ! cache.associated( call.descriptors[ 1]));
            EXPECT_TRUE( ! cache.find( transaction::id::range::global( call.trid)));
         });

         EXPECT_TRUE( cache.empty());
      }

   } // gateway::group::inbound
   
} // casual