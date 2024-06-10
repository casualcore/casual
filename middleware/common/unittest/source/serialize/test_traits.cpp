//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "casual/concepts/serialize.h"

#include "common/serialize/macro.h"

namespace casual
{
   namespace common
   {
      namespace serialize
      {
         

         TEST( casual_serialize_traits, has_serialize__false)
         {
            common::unittest::Trace trace;

            struct Value
            {
            };

            EXPECT_FALSE( ( concepts::serialize::has::serialize< Value, long>));
         }

         namespace local
         {
            namespace
            {
               struct Value
               {
                  CASUAL_CONST_CORRECT_SERIALIZE()
               };
            } // <unnamed>
         } // local

         TEST( casual_serialize_traits, has_serialize__true)
         {
            common::unittest::Trace trace;

            EXPECT_TRUE( ( concepts::serialize::has::serialize< local::Value, long>));
         }


         namespace local
         {
            namespace
            {
               namespace archive
               {
                  struct Named
                  {
                     constexpr static auto archive_properties() { return common::serialize::archive::Property::named;}
                  };

                  struct Order
                  {
                     constexpr static auto archive_properties() { return common::serialize::archive::Property::order;}
                  };
               } // archive
               
            } // <unnamed>
         } // local



         static_assert( ! archive::is::dynamic< local::archive::Named>);
         static_assert( archive::need::named< local::archive::Named>);

         static_assert( ! archive::is::dynamic< local::archive::Order>);
         static_assert( archive::need::order< local::archive::Order>);
         static_assert( ! archive::need::named< local::archive::Order>);

      } // serialize
   } // common
} // casual