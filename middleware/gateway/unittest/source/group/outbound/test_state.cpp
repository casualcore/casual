//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"


#include "gateway/group/outbound/state.h"

namespace casual
{
   using namespace common;

   namespace gateway::group::outbound
   {
      namespace local
      {
         namespace
         {
            auto fd( int value) { return strong::file::descriptor::id{ value};};

            auto lookup()
            {
               state::Lookup lookup;

               lookup.add( fd( 10), { { "a", 0}, { "b", 0}}, { { "a", 0}, {"b", 0}});
               lookup.add( fd( 20), { { "a", 0}, { "b", 0} }, { { "a", 0}, { "b", 0}});
               lookup.add( fd( 30), { { "a", 0}, { "b", 0} }, { { "a", 0}, { "b", 0}});

               lookup.add( fd( 100), { { "a", 0}, { "c", 0}}, {});
               lookup.add( fd( 101), { { "a", 0}, { "c", 0}}, {});

               lookup.add( fd( 110), { { "b", 0}, { "d", 0}}, {});
               lookup.add( fd( 111), { { "b", 0}, { "d", 0}}, {});

               lookup.add( fd( 120), { { "x", 0}}, {});


               return lookup;
            }

            template< typename L>
            auto get_connection( L& lookup, const std::string& service)
            {
               return std::get< 0>( lookup.service( service, {})).connection;
            }

            template< typename L>
            auto get_connection( L& lookup, const std::string& service, const transaction::ID& trid)
            {
               return std::get< 0>( lookup.service( service, trid)).connection;
            }

         } // <unnamed>
      } // local


      TEST( gateway_outbound_state, lookup_add)
      {
         state::Lookup lookup;

         auto advertise = lookup.add( strong::file::descriptor::id{ 9}, { { "a", 0}, { "b", 0} }, { { "a", 0}, { "b", 0}});

         EXPECT_TRUE(( advertise.services == std::vector< std::string>{ "a", "b"})) << CASUAL_NAMED_VALUE( advertise.services);
         EXPECT_TRUE(( advertise.queues == std::vector< std::string>{ "a", "b"})) << CASUAL_NAMED_VALUE( advertise.queues);
      }

      TEST( gateway_outbound_state, lookup_add__add_same_again___expect_only_1)
      {
         state::Lookup lookup;

         lookup.add( strong::file::descriptor::id{ 9}, { { "a", 0}}, {});
         ASSERT_TRUE( lookup.services().size() == 1);
         EXPECT_TRUE( lookup.services().at( "a").size() == 1);
         lookup.add( strong::file::descriptor::id{ 9}, { { "a", 0}}, {});
         ASSERT_TRUE( lookup.services().size() == 1);
         EXPECT_TRUE( lookup.services().at( "a").size() == 1);
      }

      TEST( gateway_outbound_state, lookup_add_a_2_hops_add_a_1_hops___expect_only_1_with_1_hops)
      {
         state::Lookup lookup;

         lookup.add( strong::file::descriptor::id{ 9}, { { "a", 2}}, {});
         ASSERT_TRUE( lookup.services().size() == 1);
         ASSERT_TRUE( lookup.services().at( "a").size() == 1);
         EXPECT_TRUE( lookup.services().at( "a").front().hops == 2);
         lookup.add( strong::file::descriptor::id{ 9}, { { "a", 1}}, {});
         ASSERT_TRUE( lookup.services().size() == 1);
         ASSERT_TRUE( lookup.services().at( "a").size() == 1);
         EXPECT_TRUE( lookup.services().at( "a").front().hops == 1);
      }

      TEST( gateway_outbound_state, lookup_service_a__nil_xid__expect_round_robin)
      {
         auto lookup = local::lookup();

         auto [ result, involved] = lookup.service( "a", {});
         EXPECT_TRUE( result.connection == local::fd( 10)) << CASUAL_NAMED_VALUE( result.connection);
         EXPECT_TRUE( transaction::id::null( result.trid));
         EXPECT_TRUE( ! involved);

         EXPECT_TRUE( local::get_connection( lookup, "a") == local::fd( 20)) << CASUAL_NAMED_VALUE( lookup);
         EXPECT_TRUE( local::get_connection( lookup, "a") == local::fd( 30));
         EXPECT_TRUE( local::get_connection( lookup, "a") == local::fd( 100));
         EXPECT_TRUE( local::get_connection( lookup, "a") == local::fd( 101));

         // back to the first one
         EXPECT_TRUE( local::get_connection( lookup, "a") == local::fd( 10));
      }

      TEST( gateway_outbound_state, lookup_service_a__some_with_same_xid___expect_round_robin_unless_xid_matches)
      {
         auto lookup = local::lookup();

         const auto trid = transaction::id::create();

         auto [ result, involved] = lookup.service( "a", trid);
         EXPECT_TRUE( involved);
         EXPECT_TRUE( result.connection == local::fd( 10)) << CASUAL_NAMED_VALUE( result.connection);

         const auto branched = result.trid;
         EXPECT_TRUE( branched != trid) << CASUAL_NAMED_VALUE( branched);
         // expect same global part of trid
         EXPECT_TRUE( transaction::id::range::global( branched) == transaction::id::range::global( trid));

         // 'consume' two
         {
            auto [ result, involved] = lookup.service( "a", {});
            EXPECT_TRUE( result.connection == local::fd( 20));
            EXPECT_TRUE( ! involved);
            std::tie( result, involved) = lookup.service( "a", {});
            EXPECT_TRUE( result.connection == local::fd( 30));
            EXPECT_TRUE( ! involved);
         }

         // same xid as before - expect same fd as before
         std::tie( result, involved) = lookup.service( "a", trid);
         EXPECT_TRUE( result.connection == local::fd( 10));
         EXPECT_TRUE( result.trid == branched) << CASUAL_NAMED_VALUE( result.trid);

         // expect next in the round-robin
         EXPECT_TRUE( local::get_connection( lookup, "a", {}) == local::fd( 100)) << CASUAL_NAMED_VALUE( lookup);
      }


      TEST( gateway_outbound_state, lookup_service_b_with_xid___a_with_same_xid___expect_same_connection)
      {
         auto lookup = local::lookup();

         auto xid = transaction::id::create();

         EXPECT_TRUE( local::get_connection( lookup, "c", xid) == local::fd( 100));

         // 'consume' a 
         EXPECT_TRUE( local::get_connection( lookup, "a") == local::fd( 10));

         // same xid as before - expect same fd as before - hence xid-correlation
         EXPECT_TRUE( local::get_connection( lookup, "a", xid) == local::fd( 100));
      }

      TEST( gateway_outbound_state, add_calls_to_4_services_times_4_xids__remove_all_external_xids__expect_empty_transactions_mapping)
      {
         auto lookup = local::lookup();

         auto internal_xids = array::make( transaction::id::create(), transaction::id::create(), transaction::id::create(), transaction::id::create());
         auto external_xids = std::vector< transaction::ID>{};


         // add 'calls' to a, b, c, d, with all the internal xids
         {
            for( auto& xid : internal_xids)
               for( auto& name : array::make( "a", "b", "c", "d"))
                  external_xids.push_back( std::get< 0>( lookup.service( name, xid)).trid);
         }

         // remove all externals (simulate that we've got commit/rollback for them)
         for( auto& xid : external_xids)
            lookup.remove( xid);

         EXPECT_TRUE( lookup.transactions().empty()) << CASUAL_NAMED_VALUE( lookup);
      }

      TEST( gateway_outbound_state, remove_x__expect_x_unadvertised)
      {
         auto lookup = local::lookup();

         auto xid = transaction::id::create();

         EXPECT_TRUE( local::get_connection( lookup, "x", xid) == local::fd( 120));
         auto removed = lookup.remove( local::fd( 120), { "x"}, {});

         ASSERT_TRUE( removed.services.size() == 1) << CASUAL_NAMED_VALUE( lookup);
         ASSERT_TRUE( removed.services.at( 0) == "x");

         EXPECT_TRUE( ! local::get_connection( lookup, "x", xid));
      }

      TEST( gateway_outbound_state, remove_a__expect_no_unadvertised)
      {
         auto lookup = local::lookup();

         auto xid = transaction::id::create();

         EXPECT_TRUE( local::get_connection( lookup, "a", xid) == local::fd( 10));
         auto removed = lookup.remove( local::fd( 120), { "a"}, {});

         EXPECT_TRUE( removed.services.empty()) << CASUAL_NAMED_VALUE( lookup);

         EXPECT_TRUE( local::get_connection( lookup, "a", xid));
      }

   } // gateway::group::outbound
} // casual