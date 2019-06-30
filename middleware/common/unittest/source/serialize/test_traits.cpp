//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "common/serialize/traits.h"

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

            EXPECT_FALSE( ( traits::has::serialize< Value, long>::value));
         }

         namespace local
         {
            namespace
            {
               struct Value
               {
                  template< typename A>
                  void serialize( A& archive) {}
               };
            } // <unnamed>
         } // local

         TEST( casual_serialize_traits, has_serialize__true)
         {
            common::unittest::Trace trace;

            EXPECT_TRUE( ( traits::has::serialize< local::Value, long>::value));
         }


         namespace local
         {
            namespace
            {
               struct Archive
               {
                  using need_named = void;
               };
            } // <unnamed>
         } // local

         TEST( casual_serialize_traits, need_named__true)
         {
            EXPECT_TRUE( ( traits::need::named< local::Archive>::value));
         }

         TEST( casual_serialize_traits, need_named__false)
         {
            struct Archive
            {
            };

            EXPECT_FALSE( ( traits::need::named< Archive>::value));
         }
      } // serialize
   } // common
} // casual