//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"


#include "gateway/outbound/state.h"

namespace casual
{
   using namespace common;

   namespace gateway::outbound
   {
      namespace local
      {
         namespace
         {
            auto fd( int value) { return strong::file::descriptor::id{ value};};

            auto lookup()
            {
               state::Lookup lookup;

               lookup.add( fd( 10), { "a", "b" }, { "a", "b"});
               lookup.add( fd( 20), { "a", "b" }, { "a", "b"});
               lookup.add( fd( 30), { "a", "b" }, { "a", "b"});

               lookup.add( fd( 100), { "a", "c"}, {});
               lookup.add( fd( 101), { "a", "c"}, {});

               lookup.add( fd( 110), { "b", "d"}, {});
               lookup.add( fd( 111), { "b", "d"}, {});


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


      TEST( gateway_reverse_outbound_state, lookup_add)
      {
         state::Lookup lookup;

         auto advertise = lookup.add( strong::file::descriptor::id{ 9}, { "a", "b" }, { "a", "b"});

         EXPECT_TRUE(( advertise.services == std::vector< std::string>{ "a", "b"})) << CASUAL_NAMED_VALUE( advertise.services);
         EXPECT_TRUE(( advertise.queues == std::vector< std::string>{ "a", "b"})) << CASUAL_NAMED_VALUE( advertise.queues);
      }

      TEST( gateway_reverse_outbound_state, lookup_service_a__nil_xic__expect_round_robin)
      {
         auto lookup = local::lookup();

         auto [ result, involved] = lookup.service( "a", {});
         EXPECT_TRUE( result.connection == local::fd( 10)) << CASUAL_NAMED_VALUE( result.connection);
         EXPECT_TRUE( transaction::id::null( result.trid));
         EXPECT_TRUE( ! involved);

         EXPECT_TRUE( local::get_connection( lookup, "a") == local::fd( 20));
         EXPECT_TRUE( local::get_connection( lookup, "a") == local::fd( 30));
         EXPECT_TRUE( local::get_connection( lookup, "a") == local::fd( 100));
         EXPECT_TRUE( local::get_connection( lookup, "a") == local::fd( 101));

         // back to the first one
         EXPECT_TRUE( local::get_connection( lookup, "a") == local::fd( 10));
      }

      TEST( gateway_reverse_outbound_state, lookup_service_a__some_with_same_xid___expect_round_robin_unless_xid_matches)
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


      TEST( gateway_reverse_outbound_state, lookup_service_b_with_xid___a_with_same_xid___expect_same_connection)
      {
         auto lookup = local::lookup();

         auto xid = transaction::id::create();

         EXPECT_TRUE( local::get_connection( lookup, "c", xid) == local::fd( 100));

         // 'consume' a 
         EXPECT_TRUE( local::get_connection( lookup, "a") == local::fd( 10));

         // same xid as before - expect same fd as before - hence xid-correlation
         EXPECT_TRUE( local::get_connection( lookup, "a", xid) == local::fd( 100));
      }

   } // gateway::outbound
} // casual