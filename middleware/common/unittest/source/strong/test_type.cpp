//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "common/strong/type.h"
#include "common/strong/id.h"


namespace casual
{
   namespace common
   {
      namespace local
      {
         namespace
         {
            struct policy
            {
               constexpr static int initialize() { return -1;}
               constexpr static bool valid( int value) { return value != initialize();}
            };

            using Type = strong::Type< int, policy>;

            static_assert( strong::detail::has::valid< Type>);
            static_assert( strong::detail::has::initialize< policy>);

         } // <unnamed>
      } // local
      TEST( common_strong_type, default_ctor)
      {
         common::unittest::Trace trace;

         local::Type value;

         EXPECT_TRUE( ! value);
      }

      TEST( common_strong_type, ctor)
      {
         common::unittest::Trace trace;

         local::Type value{ 42};

         EXPECT_TRUE( value);
         EXPECT_TRUE( value.value() == 42);
      }

      TEST( common_strong_type, valid)
      {
         common::unittest::Trace trace;

         EXPECT_TRUE( ! local::Type{}.valid());
         EXPECT_TRUE( local::Type{ 42}.valid());
      }

      TEST( common_strong_type, invalid_stream_nil)
      {
         common::unittest::Trace trace;
         std::ostringstream out;
         auto value = local::Type{};
         out << value;

         EXPECT_TRUE( out.str() == "nil") << out.str();
      }

      TEST( common_strong_type, in_hash_map)
      {
         common::unittest::Trace trace;

         using opt = local::Type;

         opt value;

         std::unordered_map< opt, int> map{
            { value, 1 },
            { opt( 42), 42 },
         };


         EXPECT_TRUE( map.at( value) == 1);
         EXPECT_TRUE( map.at( opt( 42)) == 42);
      }

      TEST( common_strong_type, socket_ipc)
      {
         common::unittest::Trace trace;

         auto file_descriptor = common::strong::file::descriptor::id{ 42};

         // explict ctor for file-descriptor
         auto socket = common::strong::socket::id{ file_descriptor};
         auto ipc = common::strong::ipc::descriptor::id{ file_descriptor};
         
         // implicit conversion to file-descriptor
         file_descriptor = common::strong::file::descriptor::id{ socket};
         auto ipc_file_descriptor = common::strong::file::descriptor::id{ ipc};

          EXPECT_TRUE( file_descriptor == ipc_file_descriptor); // of course.

         // EXPECT_TRUE( socket == ipc); compilation error by design
         EXPECT_TRUE( socket == file_descriptor);
         EXPECT_TRUE( ipc == file_descriptor);

      }
   } // common
} // casual
