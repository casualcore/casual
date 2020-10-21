//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <common/unittest.h>
#include <common/unittest/random/archive.h>


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
               auto expected() { return Expected;}
            };


            struct char_struct
            {
               char char_property = '0';

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( char_property);
               )
            };

            struct pod_struct
            {
               char char_property = 'A';
               int int_propertry = 42;
               long long_property = 666;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( char_property);
                  CASUAL_SERIALIZE( int_propertry);
                  CASUAL_SERIALIZE( long_property);
               )

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
            local::Holder< std::array< char, 10>, sizeof( char) * 10>,
            local::Holder< std::array< long, 10>, sizeof( long) * 10>,
            local::Holder< char[ 10][ 5], sizeof( char) * 10 * 5>,
            local::Holder< long[ 10][ 5], sizeof( long) * 10 * 5>,
            local::Holder< long[ 10][ 5][ 2], sizeof( long) * 10 * 5 * 2>,
            local::Holder< local::char_struct, sizeof( local::char_struct)>,
            local::Holder< local::pod_struct, sizeof( local::pod_struct)>,
            // tuple is not trivially copyable
            //local::Holder< std::tuple< long, double, char, short, int>, sizeof( std::tuple< long, double, char, short, int>)>,
            local::Holder< std::array< local::pod_struct, 10>, sizeof( local::pod_struct) * 10>
       >;

      TYPED_TEST_SUITE(casual_common_memory, memory_types);

      TYPED_TEST( casual_common_memory, size)
      {
         common::unittest::Trace trace;

         typename TestFixture::type current_type;

         EXPECT_TRUE( memory::size( current_type) == static_cast< platform::size::type>( TestFixture::expected()))
            << " memory::size( current_type): " <<  memory::size( current_type) << " - expected: " << TestFixture::expected();

      }


      TYPED_TEST( casual_common_memory, copy)
      {
         common::unittest::Trace trace;

         typename TestFixture::type current_type;

         platform::binary::type original;

         memory::append( current_type, original);

         // copy back to current type
         EXPECT_TRUE( memory::copy( original, 0, current_type) == memory::size( current_type));

         // append to new buffer
         platform::binary::type copied;

         memory::append( current_type, copied);

         EXPECT_TRUE( range::size( original) == memory::size( current_type));
         EXPECT_TRUE( range::size( copied) == memory::size( current_type));
         EXPECT_TRUE( original == copied); //<< "original: " << range::make( original) << " - copied: " << range::make( copied);
      }
   } // common
} // casual
