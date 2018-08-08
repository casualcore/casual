//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <common/unittest.h>


#include "common/memory.h"
#include "common/log/stream.h"

namespace casual
{
   namespace common
   {

      namespace local
      {
         namespace
         {
            template <typename T, std::size_t Expected = sizeof( T)>
            struct Holder
            {
               using type = T;
               std::size_t expected() { return Expected;}
            };


            struct char_struct
            {
               char char_property = '0';
            };

            struct pod_struct
            {
               char char_property = 'A';
               int int_propertry = 42;
               long long_property = 666;

            };


         } // <unnamed>
      } // local

      template <typename H>
      struct casual_common_memory : public ::testing::Test, public H
      {
      };


      using memory_types = ::testing::Types<
            local::Holder< char>,
            local::Holder< int>,
            local::Holder< long>,
            local::Holder< short>,
            local::Holder< float>,
            local::Holder< double>,
            local::Holder< char[ 10], sizeof( char) * 10>,
            local::Holder< long[ 10], sizeof( long) * 10>,
            local::Holder< char[ 10][ 5], sizeof( char) * 10 * 5>,
            local::Holder< long[ 10][ 5], sizeof( long) * 10 * 5>,
            local::Holder< long[ 10][ 5][ 2], sizeof( long) * 10 * 5 * 2>,
            local::Holder< local::char_struct, sizeof( local::char_struct)>,
            local::Holder< local::pod_struct, sizeof( local::pod_struct)>,
            // tuple is not trivially copyable
            //local::Holder< std::tuple< long, double, char, short, int>, sizeof( std::tuple< long, double, char, short, int>)>,
            local::Holder< local::pod_struct[ 10], sizeof( local::pod_struct) * 10>
       >;

      TYPED_TEST_CASE(casual_common_memory, memory_types);

      TYPED_TEST( casual_common_memory, size)
      {
         common::unittest::Trace trace;

         typename TestFixture::type current_type;

         EXPECT_TRUE( memory::size( current_type) == TestFixture::expected())
            << " memory::size( current_type): " <<  memory::size( current_type) << " - expected: " << TestFixture::expected();

      }

      TYPED_TEST( casual_common_memory, clear)
      {
         common::unittest::Trace trace;

         typename TestFixture::type current_type;

         memory::clear( current_type);
         
         auto first = reinterpret_cast< std::uint8_t*>( &current_type);

         EXPECT_TRUE( std::all_of( first, first + memory::size( current_type), []( std::uint8_t v){
            return v == 0;
         }));
      }


      TYPED_TEST( casual_common_memory, copy)
      {
         common::unittest::Trace trace;

         typename TestFixture::type current_type;

         platform::binary::type original;

         memory::append( current_type, original);

         //
         // make sure we don't have the same values in memory as before
         memory::clear( current_type);

         //
         // copy back to current type
         EXPECT_TRUE( memory::copy( original, 0, current_type) == memory::size( current_type));

         //
         // append to new buffer
         platform::binary::type copied;

         memory::append( current_type, copied);

         EXPECT_TRUE( original.size() == memory::size( current_type));
         EXPECT_TRUE( copied.size() == memory::size( current_type));
         EXPECT_TRUE( original == copied); //<< "original: " << range::make( original) << " - copied: " << range::make( copied);
      }
   } // common
} // casual
