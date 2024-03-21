//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include "common/unittest.h"

#include "casual/buffer/internal/field.h"
#include "casual/buffer/field.h"

#include "common/buffer/type.h"
#include "common/buffer/pool.h"

#include "common/unittest/environment.h"


#include "xatmi.h"

namespace casual
{
   enum field : long
   {

      FLD_SHORT1 = CASUAL_FIELD_SHORT * CASUAL_FIELD_TYPE_BASE + 1000 + 1,
      //const auto FLD_SHORT2 = CASUAL_FIELD_SHORT * CASUAL_FIELD_TYPE_BASE + 1000 + 2;
      //const auto FLD_SHORT3 = CASUAL_FIELD_SHORT * CASUAL_FIELD_TYPE_BASE + 1000 + 3;

      FLD_LONG1 = CASUAL_FIELD_LONG * CASUAL_FIELD_TYPE_BASE + 1000 + 1,
      //const auto FLD_LONG2 = CASUAL_FIELD_LONG * CASUAL_FIELD_TYPE_BASE + 1000 + 2;
      //const auto FLD_LONG3 = CASUAL_FIELD_LONG * CASUAL_FIELD_TYPE_BASE + 1000 + 3;

      FLD_CHAR1 = CASUAL_FIELD_CHAR * CASUAL_FIELD_TYPE_BASE + 1000 + 1,
      //const auto FLD_CHAR2 = CASUAL_FIELD_CHAR * CASUAL_FIELD_TYPE_BASE + 1000 + 2;
      //const auto FLD_CHAR3 = CASUAL_FIELD_CHAR * CASUAL_FIELD_TYPE_BASE + 1000 + 3;


      FLD_FLOAT1 = CASUAL_FIELD_FLOAT * CASUAL_FIELD_TYPE_BASE + 2000 + 1,
      //const auto FLD_FLOAT2 = CASUAL_FIELD_FLOAT * CASUAL_FIELD_TYPE_BASE + 2000 + 2;
      //const auto FLD_FLOAT3 = CASUAL_FIELD_FLOAT * CASUAL_FIELD_TYPE_BASE + 2000 + 3;

      FLD_DOUBLE1 = CASUAL_FIELD_DOUBLE * CASUAL_FIELD_TYPE_BASE + 2000 + 1,
      //const auto FLD_DOUBLE2 = CASUAL_FIELD_DOUBLE * CASUAL_FIELD_TYPE_BASE + 2000 + 2;
      //const auto FLD_DOUBLE3 = CASUAL_FIELD_DOUBLE * CASUAL_FIELD_TYPE_BASE + 2000 + 3;

      //const auto FLD_STRING1 = CASUAL_FIELD_STRING * CASUAL_FIELD_TYPE_BASE + 2000 + 1;
      //const auto FLD_STRING2 = CASUAL_FIELD_STRING * CASUAL_FIELD_TYPE_BASE + 2000 + 2;
      //const auto FLD_STRING3 = CASUAL_FIELD_STRING * CASUAL_FIELD_TYPE_BASE + 2000 + 3;

      //const auto FLD_BINARY1 = CASUAL_FIELD_BINARY * CASUAL_FIELD_TYPE_BASE + 2000 + 1;
      //const auto FLD_BINARY2 = CASUAL_FIELD_BINARY * CASUAL_FIELD_TYPE_BASE + 2000 + 2;
      //const auto FLD_BINARY3 = CASUAL_FIELD_BINARY * CASUAL_FIELD_TYPE_BASE + 2000 + 3;
   };

   namespace
   {
      class casual_field_buffer_stream : public ::testing::Test
      {
      protected:

         void SetUp() override
         {
            casual::common::environment::variable::set( "CASUAL_FIELD_TABLE", "./sample/field.ini");
         }
      };
   } //


   TEST_F( casual_field_buffer_stream, buffer_long_short)
   {
      std::stringstream stream;
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 512);
         ASSERT_TRUE( buffer != nullptr);

         EXPECT_TRUE( casual_field_add_long( &buffer, field::FLD_LONG1, 42) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( casual_field_add_short( &buffer, field::FLD_SHORT1, 10) == CASUAL_FIELD_SUCCESS);

         buffer::field::internal::stream( buffer, stream, "json");

         tpfree( buffer);
      }
      {
         auto buffer = buffer::field::internal::stream( stream, "json");
         ASSERT_TRUE( buffer != nullptr);

         short short_{};
         long long_{};
         EXPECT_TRUE( casual_field_get_long( buffer, field::FLD_LONG1, 0, &long_) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( long_ == 42);
         EXPECT_TRUE( casual_field_get_short( buffer, field::FLD_SHORT1, 0, &short_) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( short_ == 10);

         tpfree( buffer);
      }
   }


   TEST_F( casual_field_buffer_stream, buffer_char_float_double)
   {
      std::stringstream stream;
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 512);
         ASSERT_TRUE( buffer != nullptr);

         EXPECT_TRUE( casual_field_add_char( &buffer, field::FLD_CHAR1, 'A') == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( casual_field_add_float( &buffer, field::FLD_FLOAT1, 0.42f) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( casual_field_add_double( &buffer, field::FLD_DOUBLE1, 0.99) == CASUAL_FIELD_SUCCESS);

         buffer::field::internal::stream( buffer, stream, "json");

         tpfree( buffer);
      }
      {
         auto buffer = buffer::field::internal::stream( stream, "json");
         ASSERT_TRUE( buffer != nullptr);

         char chart_{};
         EXPECT_TRUE( casual_field_get_char( buffer, field::FLD_CHAR1, 0, &chart_) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( chart_ == 'A');

         float float_{};
         EXPECT_TRUE( casual_field_get_float( buffer, field::FLD_FLOAT1, 0, &float_) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( float_ > 0.419 && float_ < 0.429) << "float: " << float_ << " - stream: " << stream.str();

         double double_{};
         EXPECT_TRUE( casual_field_get_double( buffer, field::FLD_DOUBLE1, 0, &double_) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( double_ > 0.989 && double_ < 0.999) << "double: " << double_ << " - stream: " << stream.str();

         tpfree( buffer);
      }
   }

} // casual
