//
// test_field_buffer.cpp
//
//  Created on: 12 nov 2013
//      Author: Kristone
//


#include <gtest/gtest.h>

#include "common/unittest.h"

#include "buffer/field.h"
#include "common/environment.h"
#include "common/buffer/type.h"
#include "common/buffer/pool.h"

#include "xatmi.h"


#include <string>
#include <array>

namespace casual
{

   namespace common
   {
      namespace
      {

         const auto FLD_SHORT1 = CASUAL_FIELD_SHORT * CASUAL_FIELD_TYPE_BASE + 1000 + 1;
         const auto FLD_SHORT2 = CASUAL_FIELD_SHORT * CASUAL_FIELD_TYPE_BASE + 1000 + 2;
         //const auto FLD_SHORT3 = CASUAL_FIELD_SHORT * CASUAL_FIELD_TYPE_BASE + 1000 + 3;

         const auto FLD_LONG1 = CASUAL_FIELD_LONG * CASUAL_FIELD_TYPE_BASE + 1000 + 1;
         //const auto FLD_LONG2 = CASUAL_FIELD_LONG * CASUAL_FIELD_TYPE_BASE + 1000 + 2;
         //const auto FLD_LONG3 = CASUAL_FIELD_LONG * CASUAL_FIELD_TYPE_BASE + 1000 + 3;

         const auto FLD_CHAR1 = CASUAL_FIELD_CHAR * CASUAL_FIELD_TYPE_BASE + 1000 + 1;
         //const auto FLD_CHAR2 = CASUAL_FIELD_CHAR * CASUAL_FIELD_TYPE_BASE + 1000 + 2;
         //const auto FLD_CHAR3 = CASUAL_FIELD_CHAR * CASUAL_FIELD_TYPE_BASE + 1000 + 3;


         const auto FLD_FLOAT1 = CASUAL_FIELD_FLOAT * CASUAL_FIELD_TYPE_BASE + 2000 + 1;
         //const auto FLD_FLOAT2 = CASUAL_FIELD_FLOAT * CASUAL_FIELD_TYPE_BASE + 2000 + 2;
         //const auto FLD_FLOAT3 = CASUAL_FIELD_FLOAT * CASUAL_FIELD_TYPE_BASE + 2000 + 3;

         const auto FLD_DOUBLE1 = CASUAL_FIELD_DOUBLE * CASUAL_FIELD_TYPE_BASE + 2000 + 1;
         const auto FLD_DOUBLE2 = CASUAL_FIELD_DOUBLE * CASUAL_FIELD_TYPE_BASE + 2000 + 2;
         const auto FLD_DOUBLE3 = CASUAL_FIELD_DOUBLE * CASUAL_FIELD_TYPE_BASE + 2000 + 3;

         const auto FLD_STRING1 = CASUAL_FIELD_STRING * CASUAL_FIELD_TYPE_BASE + 2000 + 1;
         const auto FLD_STRING2 = CASUAL_FIELD_STRING * CASUAL_FIELD_TYPE_BASE + 2000 + 2;
         //const auto FLD_STRING3 = CASUAL_FIELD_STRING * CASUAL_FIELD_TYPE_BASE + 2000 + 3;

         const auto FLD_BINARY1 = CASUAL_FIELD_BINARY * CASUAL_FIELD_TYPE_BASE + 2000 + 1;
         //const auto FLD_BINARY2 = CASUAL_FIELD_BINARY * CASUAL_FIELD_TYPE_BASE + 2000 + 2;
         //const auto FLD_BINARY3 = CASUAL_FIELD_BINARY * CASUAL_FIELD_TYPE_BASE + 2000 + 3;
      }


      TEST( casual_field_buffer, pool)
      {
         auto type = common::buffer::type::combine( CASUAL_FIELD);
         auto handle = buffer::pool::Holder::instance().allocate( type, 666);

         auto buffer = buffer::pool::Holder::instance().get( handle, 0);

         EXPECT_TRUE( buffer.transport == 0) << "transport: " <<  buffer.transport;
         EXPECT_TRUE( buffer.reserved == 666) << "reserved: " <<  buffer.reserved;
         EXPECT_TRUE( buffer.payload().type == type) << "buffer.type: " << buffer.payload().type;

         buffer::pool::Holder::instance().deallocate( handle);
      }


      TEST( casual_field_buffer, tptypes)
      {
         common::unittest::Trace trace;

         char* buffer = tpalloc( CASUAL_FIELD, nullptr, 666);
         ASSERT_TRUE( buffer != 0);

         std::array< char, 8> type;
         std::array< char, 16> subtype;

         auto size = tptypes( buffer, type.data(), subtype.data());

         EXPECT_TRUE( size == 666) << "size: " << size;
         EXPECT_TRUE( std::string( type.data()) == CASUAL_FIELD) << "type.data(): " << type.data();
         EXPECT_TRUE( std::string( subtype.data()).empty()) << "subtype.data(): " << subtype.data();

         tpfree( buffer);
      }


      TEST( casual_field_buffer, use_with_invalid_buffer__expecting_invalid_buffer)
      {
         char* invalid = nullptr;

         EXPECT_TRUE( casual_field_explore_buffer( nullptr, nullptr, nullptr) == CASUAL_FIELD_INVALID_HANDLE);

         EXPECT_TRUE( casual_field_explore_buffer( invalid, nullptr, nullptr) == CASUAL_FIELD_INVALID_HANDLE);

         EXPECT_TRUE( casual_field_add_char( &invalid, FLD_CHAR1, 'a') == CASUAL_FIELD_INVALID_HANDLE);
         EXPECT_TRUE( casual_field_add_short( &invalid, FLD_SHORT1, 123) == CASUAL_FIELD_INVALID_HANDLE);
         EXPECT_TRUE( casual_field_add_long( &invalid, FLD_LONG1, 123456) == CASUAL_FIELD_INVALID_HANDLE);
         EXPECT_TRUE( casual_field_add_float( &invalid, FLD_FLOAT1, 123.456) == CASUAL_FIELD_INVALID_HANDLE);
         EXPECT_TRUE( casual_field_add_double( &invalid, FLD_DOUBLE1, 123456.789) == CASUAL_FIELD_INVALID_HANDLE);
         EXPECT_TRUE( casual_field_add_string( &invalid, FLD_STRING1, "Casual rules!") == CASUAL_FIELD_INVALID_HANDLE);
         EXPECT_TRUE( casual_field_add_binary( &invalid, FLD_BINARY1, "!#¤%&", 5) == CASUAL_FIELD_INVALID_HANDLE);

         EXPECT_TRUE( casual_field_get_char( invalid, FLD_CHAR1, 0, nullptr) == CASUAL_FIELD_INVALID_HANDLE);
         EXPECT_TRUE( casual_field_get_short( invalid, FLD_SHORT1, 0, nullptr) == CASUAL_FIELD_INVALID_HANDLE);
         EXPECT_TRUE( casual_field_get_long( invalid, FLD_LONG1, 0, nullptr) == CASUAL_FIELD_INVALID_HANDLE);
         EXPECT_TRUE( casual_field_get_float( invalid, FLD_FLOAT1, 0, nullptr) == CASUAL_FIELD_INVALID_HANDLE);
         EXPECT_TRUE( casual_field_get_double( invalid, FLD_DOUBLE1, 0, nullptr) == CASUAL_FIELD_INVALID_HANDLE);
         EXPECT_TRUE( casual_field_get_string( invalid, FLD_STRING1, 0, nullptr) == CASUAL_FIELD_INVALID_HANDLE);
         EXPECT_TRUE( casual_field_get_binary( invalid, FLD_BINARY1, 0, nullptr, nullptr) == CASUAL_FIELD_INVALID_HANDLE);

         EXPECT_TRUE( casual_field_explore_value( invalid, FLD_CHAR1, 0, nullptr) == CASUAL_FIELD_INVALID_HANDLE);

         EXPECT_TRUE( casual_field_remove_all( invalid) == CASUAL_FIELD_INVALID_HANDLE);

         EXPECT_TRUE( casual_field_remove_id( invalid, FLD_CHAR1) == CASUAL_FIELD_INVALID_HANDLE);

         EXPECT_TRUE( casual_field_remove_occurrence( invalid, FLD_CHAR1, 0) == CASUAL_FIELD_INVALID_HANDLE);

         auto buffer = tpalloc( CASUAL_FIELD, "", 0);
         ASSERT_TRUE( buffer != nullptr);

         EXPECT_TRUE( casual_field_copy_buffer( &invalid, buffer) == CASUAL_FIELD_INVALID_HANDLE);
         EXPECT_TRUE( casual_field_copy_buffer( &buffer, invalid) == CASUAL_FIELD_INVALID_HANDLE);

         tpfree( buffer);

         long id = CASUAL_FIELD_NO_ID;
         long occurrence;
         EXPECT_TRUE( casual_field_next( invalid, &id, &occurrence) == CASUAL_FIELD_INVALID_HANDLE);

      }

      TEST( casual_field_buffer, use_with_wrong_buffer__expecting_invalid_buffer)
      {
         auto buffer = tpalloc( CASUAL_BUFFER_BINARY_TYPE, CASUAL_BUFFER_BINARY_SUBTYPE, 128);
         ASSERT_TRUE( buffer != nullptr);

         EXPECT_TRUE( casual_field_add_char( &buffer, FLD_CHAR1, 'a') == CASUAL_FIELD_INVALID_HANDLE);

         tpfree( buffer);
      }

      TEST( casual_field_buffer, add_binary_data_with_negative_size__expecting_invalid_argument)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 0);
         ASSERT_TRUE( buffer != nullptr);

         EXPECT_TRUE( casual_field_add_binary( &buffer, FLD_BINARY1, "some data", -123) == CASUAL_FIELD_INVALID_ARGUMENT);

         tpfree( buffer);
      }

      TEST( casual_field_buffer, allocate_with_zero_size__expecting_success)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 0);
         ASSERT_TRUE( buffer != nullptr);

         long used = 0;
         EXPECT_TRUE( casual_field_explore_buffer( buffer, nullptr, &used) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( used == 0);
         tpfree( buffer);
      }

      TEST( casual_field_buffer, reallocate_with_larger_size__expecting_success)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 64);
         ASSERT_TRUE( buffer != nullptr);

         long initial_size = 0;
         EXPECT_TRUE( casual_field_explore_buffer( buffer, &initial_size, nullptr) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( initial_size == 64);

         buffer = tprealloc( buffer, 128);
         ASSERT_TRUE( buffer != nullptr);

         long updated_size = 0;
         EXPECT_TRUE( casual_field_explore_buffer( buffer, &updated_size, nullptr) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( updated_size == 128);

         tpfree( buffer);
      }


      TEST( casual_field_buffer, reallocate_with_smaller_size__expecting_success)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 128);
         ASSERT_TRUE( buffer != nullptr);

         long initial_size = 0;
         EXPECT_TRUE( casual_field_explore_buffer( buffer, &initial_size, nullptr) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( initial_size == 128);

         EXPECT_TRUE( casual_field_add_string( &buffer, FLD_STRING1, "goin' casual today!") == CASUAL_FIELD_SUCCESS);

         buffer = tprealloc( buffer, 10);
         ASSERT_TRUE( buffer != nullptr);

         long updated_size = 0;
         EXPECT_TRUE( casual_field_explore_buffer( buffer, &updated_size, nullptr) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( updated_size < 128);

         buffer = tprealloc( buffer, 0);
         ASSERT_TRUE( buffer != nullptr);

         long final_size = 0;
         long final_used = 0;
         EXPECT_TRUE( casual_field_explore_buffer( buffer, &final_size, &final_used) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( final_size < 64);
         EXPECT_TRUE( final_size > 0);
         EXPECT_TRUE( final_size == final_used);

         tpfree( buffer);
      }

      TEST( casual_field_buffer, reallocate_with_invalid_handle__expecting_nullptr)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 128);
         ASSERT_TRUE( buffer != nullptr);

         tpfree( buffer);

         buffer = tprealloc( buffer, 0);
         EXPECT_TRUE( buffer == nullptr);
      }


      TEST( casual_field_buffer, allocate_with_small_size_and_write_much__expecting_increased_size)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 64);
         ASSERT_TRUE( buffer != nullptr);

         const char string[] = "A long string that is longer than what would fit in this small buffer";

         EXPECT_TRUE( casual_field_add_string( &buffer, FLD_STRING1, string) == CASUAL_FIELD_SUCCESS);

         EXPECT_TRUE( tptypes( buffer, nullptr, nullptr) > 64);

         tpfree( buffer);
      }


      TEST( casual_field_buffer, add_and_get__expecting_success)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 512);
         ASSERT_TRUE( buffer != nullptr);

         EXPECT_TRUE( casual_field_add_char( &buffer, FLD_CHAR1, 'a') == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( casual_field_add_short( &buffer, FLD_SHORT1, 123) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( casual_field_add_long( &buffer, FLD_LONG1, 654321) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( casual_field_add_float( &buffer, FLD_FLOAT1, 3.14) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( casual_field_add_double( &buffer, FLD_DOUBLE1, 987.654) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( casual_field_add_string( &buffer, FLD_STRING1, "Hello!") == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( casual_field_add_binary( &buffer, FLD_BINARY1, "Some Data", 9) == CASUAL_FIELD_SUCCESS);

         {
            char value{};
            EXPECT_TRUE( casual_field_get_char( buffer, FLD_CHAR1, 0, &value) == CASUAL_FIELD_SUCCESS);
            EXPECT_TRUE( value == 'a') << value;
         }

         {
            short value{};
            EXPECT_TRUE( casual_field_get_short( buffer, FLD_SHORT1, 0, &value) == CASUAL_FIELD_SUCCESS);
            EXPECT_TRUE( value == 123) << value;
         }

         {
            long value{};
            EXPECT_TRUE( casual_field_get_long( buffer, FLD_LONG1, 0, &value) == CASUAL_FIELD_SUCCESS);
            EXPECT_TRUE( value == 654321) << value;
         }

         {
            float value{};
            EXPECT_TRUE( casual_field_get_float( buffer, FLD_FLOAT1, 0, &value) == CASUAL_FIELD_SUCCESS);
            EXPECT_TRUE( value > 3.1 && value < 3.2) << value;
         }

         {
            double value{};
            EXPECT_TRUE( casual_field_get_double( buffer, FLD_DOUBLE1, 0, &value) == CASUAL_FIELD_SUCCESS);
            EXPECT_TRUE( value > 987.6 && value < 987.7) << value;
         }

         {
            const char* value{};
            EXPECT_TRUE( casual_field_get_string( buffer, FLD_STRING1, 0, &value) == CASUAL_FIELD_SUCCESS);
            EXPECT_STREQ( value, "Hello!") << value;
         }

         {
            const char* data{};
            long size{};
            EXPECT_TRUE( casual_field_get_binary( buffer, FLD_BINARY1, 0, &data, &size) == CASUAL_FIELD_SUCCESS);
            EXPECT_TRUE( std::string( data, size) == "Some Data") << std::string( data, size);
            EXPECT_TRUE( size == 9) << size;
         }

         tpfree( buffer);
      }


      TEST( casual_field_buffer, add_and_get_several_occurrences__expecting_success)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 512);
         ASSERT_TRUE( buffer != nullptr);

         EXPECT_TRUE( casual_field_add_short( &buffer, FLD_SHORT1, 123) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( casual_field_add_string( &buffer, FLD_STRING1, "Hello!") == CASUAL_FIELD_SUCCESS);

         EXPECT_TRUE( casual_field_add_short( &buffer, FLD_SHORT1, 321) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( casual_field_add_string( &buffer, FLD_STRING1, "!olleH") == CASUAL_FIELD_SUCCESS);



         short short_integer;
         EXPECT_TRUE( casual_field_get_short( buffer, FLD_SHORT1, 0, &short_integer) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( short_integer == 123);

         EXPECT_TRUE( casual_field_get_short( buffer, FLD_SHORT1, 1, &short_integer) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( short_integer == 321);


         const char* string;
         EXPECT_TRUE( casual_field_get_string( buffer, FLD_STRING1, 0, &string) == CASUAL_FIELD_SUCCESS);
         EXPECT_STREQ( string, "Hello!") << string;

         EXPECT_TRUE( casual_field_get_string( buffer, FLD_STRING1, 1, &string) == CASUAL_FIELD_SUCCESS);
         EXPECT_STREQ( string, "!olleH") << string;

         tpfree( buffer);
      }


      TEST( casual_field_buffer, add_one_and_get_two_occurrences__expecting_no_occurrence)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 512);
         ASSERT_TRUE( buffer != nullptr);

         EXPECT_TRUE( casual_field_add_short( &buffer, FLD_SHORT1, 123) == CASUAL_FIELD_SUCCESS);

         short short_integer;
         EXPECT_TRUE( casual_field_get_short( buffer, FLD_SHORT1, 0, &short_integer) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( casual_field_get_short( buffer, FLD_SHORT1, 1, &short_integer) == CASUAL_FIELD_OUT_OF_BOUNDS);

         tpfree( buffer);
      }

      TEST( casual_field_buffer, add_short_with_id_of_long__expecting_invalid_argument)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 512);
         ASSERT_TRUE( buffer != nullptr);

         EXPECT_TRUE( casual_field_add_short( &buffer, FLD_LONG1, 123) == CASUAL_FIELD_INVALID_ARGUMENT);

         tpfree( buffer);
      }

      TEST( casual_field_buffer, add_short_with_invalid_id__expecting_invalid_id)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 512);
         ASSERT_TRUE( buffer != nullptr);

         EXPECT_TRUE( casual_field_add_short( &buffer, -1, 123) == CASUAL_FIELD_INVALID_ARGUMENT);
         EXPECT_TRUE( casual_field_add_short( &buffer, 0, 123) == CASUAL_FIELD_INVALID_ARGUMENT);

         tpfree( buffer);
      }


      TEST( casual_field_buffer, get_type_from_id__expecting_success)
      {
         int type;
         EXPECT_TRUE( casual_field_type_of_id( FLD_LONG1, &type) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( type == CASUAL_FIELD_LONG);
      }

      TEST( casual_field_buffer, type_from_invalid_id__expecting_invalid_id)
      {
         int type;
         EXPECT_TRUE( casual_field_type_of_id( 0, &type) == CASUAL_FIELD_INVALID_ARGUMENT);
         EXPECT_TRUE( casual_field_type_of_id( -1, &type) == CASUAL_FIELD_INVALID_ARGUMENT);
      }

      TEST( casual_field_buffer, remove_occurrence_with_invalid_id__expecting_invalid_argument)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 512);
         ASSERT_TRUE( buffer != nullptr);

         EXPECT_TRUE( casual_field_remove_occurrence( buffer, 0, 0) == CASUAL_FIELD_INVALID_ARGUMENT);
         EXPECT_TRUE( casual_field_remove_occurrence( buffer, -1, 0) == CASUAL_FIELD_INVALID_ARGUMENT);

         tpfree( buffer);
      }


      TEST( casual_field_buffer, remove_occurrence_with_no_occurrence__expecting_no_occurrence)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 512);
         ASSERT_TRUE( buffer != nullptr);

         EXPECT_TRUE( casual_field_explore_value( buffer, FLD_LONG1, 0, nullptr) == CASUAL_FIELD_OUT_OF_BOUNDS);
         EXPECT_TRUE( casual_field_remove_occurrence( buffer, FLD_LONG1, 0) == CASUAL_FIELD_OUT_OF_BOUNDS);

         tpfree( buffer);
      }

      TEST( casual_field_buffer, add_and_remove_occurrence__expecting_success)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 512);
         ASSERT_TRUE( buffer != nullptr);

         EXPECT_TRUE( casual_field_add_short( &buffer, FLD_SHORT1, 123) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( casual_field_remove_occurrence( buffer, FLD_SHORT1, 0) == CASUAL_FIELD_SUCCESS);

         tpfree( buffer);
      }

      TEST( casual_field_buffer, add_two_and_remove_first_occurrence_and_get_second_as_first__expecting_success)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 512);
         ASSERT_TRUE( buffer != nullptr);

         EXPECT_TRUE( casual_field_add_short( &buffer, FLD_SHORT1, 123) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( casual_field_add_short( &buffer, FLD_SHORT1, 321) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( casual_field_remove_occurrence( buffer, FLD_SHORT1, 0) == CASUAL_FIELD_SUCCESS);
         short short_integer;
         EXPECT_TRUE( casual_field_get_short( buffer, FLD_SHORT1, 0, &short_integer) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( short_integer == 321);

         tpfree( buffer);
      }

      TEST( casual_field_buffer, add_twp_and_remove_second_occurrence_and_first_second_as_first__expecting_success)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 512);
         ASSERT_TRUE( buffer != nullptr);

         EXPECT_TRUE( casual_field_add_short( &buffer, FLD_SHORT1, 123) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( casual_field_add_short( &buffer, FLD_SHORT1, 321) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( casual_field_remove_occurrence( buffer, FLD_SHORT1, 1) == CASUAL_FIELD_SUCCESS);
         short short_integer;
         EXPECT_TRUE( casual_field_get_short( buffer, FLD_SHORT1, 0, &short_integer) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( short_integer == 123);

         tpfree( buffer);
      }


      TEST( casual_field_buffer, remove_all_occurrences_and_check_existence__expecting_no_occurrence)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 512);
         ASSERT_TRUE( buffer != nullptr);

         EXPECT_FALSE( casual_field_add_short( &buffer, FLD_SHORT1, 123));
         EXPECT_FALSE( casual_field_add_short( &buffer, FLD_SHORT1, 321));
         EXPECT_FALSE( casual_field_explore_value( buffer, FLD_SHORT1, 0, nullptr));
         EXPECT_FALSE( casual_field_remove_id( buffer, FLD_SHORT1));
         EXPECT_TRUE( casual_field_explore_value( buffer, FLD_SHORT1, 0, nullptr) == CASUAL_FIELD_OUT_OF_BOUNDS);

         tpfree( buffer);
      }

      TEST( casual_field_buffer, add_to_source_and_copy_buffer_and_get_from_target__expecting_success)
      {
         auto source = tpalloc( CASUAL_FIELD, "", 64);
         ASSERT_TRUE( source != nullptr);

         auto target = tpalloc( CASUAL_FIELD, "", 128);
         ASSERT_TRUE( target != nullptr);


         EXPECT_TRUE( casual_field_add_short( &source, FLD_SHORT1, 123) == CASUAL_FIELD_SUCCESS);
         ASSERT_TRUE( casual_field_copy_buffer( &target, source) == CASUAL_FIELD_SUCCESS);

         {
            long size = 0;
            casual_field_explore_buffer( source, &size, nullptr);
            EXPECT_TRUE( size == 64) << size;
         }

         {
            long size = 0;
            casual_field_explore_buffer( target, &size, nullptr);
            EXPECT_TRUE( size == 128) << size;
         }

         short short_integer;
         EXPECT_TRUE( casual_field_get_short( target, FLD_SHORT1, 0, &short_integer) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( short_integer == 123);


         tpfree( source);
         tpfree( target);
      }


      TEST( casual_field_buffer, test_some_iteration)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 128);

         ASSERT_TRUE( buffer != nullptr);

         ASSERT_TRUE( casual_field_add_short( &buffer, FLD_SHORT1, 123) == CASUAL_FIELD_SUCCESS);
         ASSERT_TRUE( casual_field_add_long( &buffer, FLD_LONG1, 123456) == CASUAL_FIELD_SUCCESS);
         ASSERT_TRUE( casual_field_add_long( &buffer, FLD_LONG1, 654321) == CASUAL_FIELD_SUCCESS);
         ASSERT_TRUE( casual_field_add_short( &buffer, FLD_SHORT1, 456) == CASUAL_FIELD_SUCCESS);
         ASSERT_TRUE( casual_field_add_long( &buffer, FLD_LONG1, 987654321) == CASUAL_FIELD_SUCCESS);

         long id = CASUAL_FIELD_NO_ID;
         long occurrence = 0;

         EXPECT_TRUE( casual_field_next( buffer, &id, &occurrence) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( id == FLD_SHORT1) << id;
         EXPECT_TRUE( occurrence == 0) << occurrence;
         EXPECT_TRUE( casual_field_next( buffer, &id, &occurrence) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( id == FLD_LONG1) << id;
         EXPECT_TRUE( occurrence == 0) << occurrence;
         EXPECT_TRUE( casual_field_next( buffer, &id, &occurrence) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( id == FLD_LONG1) << id;
         EXPECT_TRUE( occurrence == 1) << occurrence;
         EXPECT_TRUE( casual_field_next( buffer, &id, &occurrence) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( id == FLD_SHORT1) << id;
         EXPECT_TRUE( occurrence == 1) << occurrence;
         EXPECT_TRUE( casual_field_next( buffer, &id, &occurrence) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( id == FLD_LONG1) << id;
         EXPECT_TRUE( occurrence == 2) << occurrence;
         const auto result = casual_field_next( buffer, &id, &occurrence);
         EXPECT_TRUE( result == CASUAL_FIELD_OUT_OF_BOUNDS) << result;

         tpfree( buffer);

      }

      namespace
      {
         class casual_field_buffer_repository : public ::testing::Test
         {
         protected:

            void SetUp() override
            {
               /* This is a semi-isolated-unittest */
               //casual::common::environment::variable::set( "CASUAL_FIELD_TABLE", "./sample/field.xml");
               //casual::common::environment::variable::set( "CASUAL_FIELD_TABLE", "./sample/field.json");
               //casual::common::environment::variable::set( "CASUAL_FIELD_TABLE", "./sample/field.ini");
               casual::common::environment::variable::set( "CASUAL_FIELD_TABLE", "./sample/field.yaml");
            }

            void TearDown() override
            {
               //casual::common::environment::variable::unset( "CASUAL_FIELD_TABLE");
            }

         };
      }


      TEST_F( casual_field_buffer_repository, get_name_from_id_from_group_one__expecting_success)
      {
         const char* name;
         ASSERT_TRUE( casual_field_name_of_id( FLD_SHORT1, &name) == CASUAL_FIELD_SUCCESS);
         EXPECT_STREQ( name, "FLD_SHORT1");
      }

      TEST_F( casual_field_buffer_repository, get_name_from_id_to_null__expecting_success_and_no_crasch)
      {
         ASSERT_TRUE( casual_field_name_of_id( FLD_SHORT1, nullptr) == CASUAL_FIELD_SUCCESS);
      }

      TEST_F( casual_field_buffer_repository, get_name_from_id_from_group_two__expecting_success)
      {
         const char* name;
         ASSERT_TRUE( casual_field_name_of_id( FLD_DOUBLE2, &name) == CASUAL_FIELD_SUCCESS);
         EXPECT_STREQ( name, "FLD_DOUBLE2");
      }

      TEST_F( casual_field_buffer_repository, get_id_from_name_from_group_one__expecting_success)
      {
         long id;
         ASSERT_TRUE( casual_field_id_of_name( "FLD_SHORT2", &id) == CASUAL_FIELD_SUCCESS);
         EXPECT_EQ( id, FLD_SHORT2);
      }

      TEST_F( casual_field_buffer_repository, get_id_from_name_to_null__expecting_success_and_no_crasch)
      {
         ASSERT_TRUE( casual_field_id_of_name( "FLD_SHORT2", nullptr) == CASUAL_FIELD_SUCCESS);
      }

      TEST_F( casual_field_buffer_repository, get_id_from_name_from_group_two__expecting_success)
      {
         long id;
         ASSERT_TRUE( casual_field_id_of_name( "FLD_DOUBLE3", &id) == CASUAL_FIELD_SUCCESS);
         EXPECT_EQ( id, FLD_DOUBLE3);
      }

      TEST_F( casual_field_buffer_repository, get_name_from_non_existing_id__expecting_unkown_id)
      {
         const char* name;
         ASSERT_TRUE( casual_field_name_of_id( 666, &name) == CASUAL_FIELD_OUT_OF_BOUNDS);
      }

      TEST_F( casual_field_buffer_repository, get_id_from_non_existing_name__expecting_unknown_id)
      {
         long id;
         ASSERT_TRUE( casual_field_id_of_name( "NON_EXISTING_NAME", &id) == CASUAL_FIELD_OUT_OF_BOUNDS);
      }

      TEST_F( casual_field_buffer_repository, get_name_from_invalid_id__expecting_invalid_argument)
      {
         const char* name;
         ASSERT_TRUE( casual_field_name_of_id( -666, &name) == CASUAL_FIELD_INVALID_ARGUMENT);
      }

      TEST_F( casual_field_buffer_repository, get_id_from_invalid_name__expecting_invalid_argument)
      {
         long id;
         ASSERT_TRUE( casual_field_id_of_name( nullptr, &id) == CASUAL_FIELD_INVALID_ARGUMENT);
      }

      TEST_F( casual_field_buffer_repository, get_name_of_type__expecting_success)
      {
         const char* name = nullptr;
         EXPECT_TRUE( casual_field_name_of_type( CASUAL_FIELD_SHORT, &name) == CASUAL_FIELD_SUCCESS);
         EXPECT_STREQ( name, "short");
         EXPECT_TRUE( casual_field_name_of_type( CASUAL_FIELD_LONG, &name) == CASUAL_FIELD_SUCCESS);
         EXPECT_STREQ( name, "long");
         EXPECT_TRUE( casual_field_name_of_type( CASUAL_FIELD_CHAR, &name) == CASUAL_FIELD_SUCCESS);
         EXPECT_STREQ( name, "char");
         EXPECT_TRUE( casual_field_name_of_type( CASUAL_FIELD_FLOAT, &name) == CASUAL_FIELD_SUCCESS);
         EXPECT_STREQ( name, "float");
         EXPECT_TRUE( casual_field_name_of_type( CASUAL_FIELD_DOUBLE, &name) == CASUAL_FIELD_SUCCESS);
         EXPECT_STREQ( name, "double");
         EXPECT_TRUE( casual_field_name_of_type( CASUAL_FIELD_STRING, &name) == CASUAL_FIELD_SUCCESS);
         EXPECT_STREQ( name, "string");
         EXPECT_TRUE( casual_field_name_of_type( CASUAL_FIELD_BINARY, &name) == CASUAL_FIELD_SUCCESS);
         EXPECT_STREQ( name, "binary");
      }

      TEST_F( casual_field_buffer_repository, get_name_of_invalid_type__expecting_invalid_argument)
      {
         const char* name = nullptr;
         EXPECT_TRUE( casual_field_name_of_type( 666, &name) == CASUAL_FIELD_INVALID_ARGUMENT);
      }

      TEST_F( casual_field_buffer_repository, get_name_of_type_to_null__expecting_success_and_no_crasch)
      {
         EXPECT_TRUE( casual_field_name_of_type( CASUAL_FIELD_STRING, nullptr) == CASUAL_FIELD_SUCCESS);
      }

      TEST_F( casual_field_buffer_repository, get_type_of_name__expecting_success)
      {
         int type = -1;
         EXPECT_TRUE( casual_field_type_of_name( "short", &type) == CASUAL_FIELD_SUCCESS);
         EXPECT_EQ( type, CASUAL_FIELD_SHORT);
         EXPECT_TRUE( casual_field_type_of_name( "long", &type) == CASUAL_FIELD_SUCCESS);
         EXPECT_EQ( type, CASUAL_FIELD_LONG);
         EXPECT_TRUE( casual_field_type_of_name( "char", &type) == CASUAL_FIELD_SUCCESS);
         EXPECT_EQ( type, CASUAL_FIELD_CHAR);
         EXPECT_TRUE( casual_field_type_of_name( "float", &type) == CASUAL_FIELD_SUCCESS);
         EXPECT_EQ( type, CASUAL_FIELD_FLOAT);
         EXPECT_TRUE( casual_field_type_of_name( "double", &type) == CASUAL_FIELD_SUCCESS);
         EXPECT_EQ( type, CASUAL_FIELD_DOUBLE);
         EXPECT_TRUE( casual_field_type_of_name( "string", &type) == CASUAL_FIELD_SUCCESS);
         EXPECT_EQ( type, CASUAL_FIELD_STRING);
         EXPECT_TRUE( casual_field_type_of_name( "binary", &type) == CASUAL_FIELD_SUCCESS);
         EXPECT_EQ( type, CASUAL_FIELD_BINARY);
      }

      TEST_F( casual_field_buffer_repository, get_type_of_invalid_name__expecting_invalid_argument)
      {
         int type = -1;
         EXPECT_TRUE( casual_field_type_of_name( "666", &type) == CASUAL_FIELD_INVALID_ARGUMENT);
      }

      TEST_F( casual_field_buffer_repository, get_type_of_name_to_null__expecting_success_and_no_crasch)
      {
         EXPECT_TRUE( casual_field_type_of_name( "string", nullptr) == CASUAL_FIELD_SUCCESS);
      }

      TEST_F( casual_field_buffer_repository, print__expecting_success)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 512);
         ASSERT_TRUE( buffer != nullptr);

         EXPECT_FALSE( casual_field_add_string( &buffer, FLD_STRING1, "First string 1"));
         EXPECT_FALSE( casual_field_add_string( &buffer, FLD_STRING2, "First string 2"));
         EXPECT_FALSE( casual_field_add_string( &buffer, FLD_STRING1, "Other string 1"));
         EXPECT_FALSE( casual_field_add_float( &buffer, FLD_FLOAT1, 3.14));

         EXPECT_FALSE( casual_field_print( buffer));

         tpfree( buffer);
      }

      TEST_F( casual_field_buffer_repository, DISABLED_match__expecting_match)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 512);
         ASSERT_TRUE( buffer != nullptr);

         ASSERT_FALSE( casual_field_add_string( &buffer, FLD_STRING1, "First string 1"));
         ASSERT_FALSE( casual_field_add_string( &buffer, FLD_STRING2, "First string 2"));
         ASSERT_FALSE( casual_field_add_string( &buffer, FLD_STRING1, "Other string 1"));
         ASSERT_FALSE( casual_field_add_float( &buffer, FLD_FLOAT1, 3.14));

         int match = 0;
         ASSERT_FALSE( casual_field_match( buffer, R"x(FLD_STRING1\[1\] = Other string)x", &match));

         EXPECT_TRUE( match);

         tpfree( buffer);
      }

      TEST_F( casual_field_buffer_repository, DISABLED_match__expecting_optimized_match)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 512);
         ASSERT_TRUE( buffer != nullptr);

         ASSERT_FALSE( casual_field_add_string( &buffer, FLD_STRING1, "First string 1"));
         ASSERT_FALSE( casual_field_add_string( &buffer, FLD_STRING2, "First string 2"));
         ASSERT_FALSE( casual_field_add_string( &buffer, FLD_STRING1, "Other string 1"));
         ASSERT_FALSE( casual_field_add_float( &buffer, FLD_FLOAT1, 3.14));


         const void* regex = 0;
         ASSERT_FALSE( casual_field_make_expression( R"x(FLD_STRING1\[1\] = Other string)x", &regex));

         int match = 0;
         ASSERT_FALSE( casual_field_match_expression( buffer, regex, &match));

         EXPECT_TRUE( match);

         ASSERT_FALSE( casual_field_free_expression( regex));

         tpfree( buffer);
      }


      TEST( casual_field_buffer, add_three_remove_two_field_and_then_iterate__expecting_third_as_first)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 512);
         ASSERT_TRUE( buffer != nullptr);

         EXPECT_TRUE( casual_field_add_short( &buffer, FLD_SHORT1, 123) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( casual_field_add_long( &buffer, FLD_LONG1, 123456) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( casual_field_add_float( &buffer, FLD_FLOAT1, 3.14) == CASUAL_FIELD_SUCCESS);

         EXPECT_TRUE( casual_field_remove_occurrence( buffer, FLD_SHORT1, 0) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( casual_field_remove_occurrence( buffer, FLD_LONG1, 0) == CASUAL_FIELD_SUCCESS);

         long id = CASUAL_FIELD_NO_ID;
         long occurrence;

         EXPECT_TRUE( casual_field_next( buffer, &id, &occurrence) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( id == FLD_FLOAT1) << id;
         EXPECT_TRUE( occurrence == 0);

         EXPECT_TRUE( casual_field_next( buffer, &id, &occurrence) == CASUAL_FIELD_OUT_OF_BOUNDS);

         tpfree( buffer);
      }


      TEST( casual_field_buffer, add_three_remove_middle_field_and_then_iterate__expecting_third_as_second)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 512);
         ASSERT_TRUE( buffer != nullptr);

         EXPECT_TRUE( casual_field_add_short( &buffer, FLD_SHORT1, 123) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( casual_field_add_long( &buffer, FLD_LONG1, 123456) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( casual_field_add_float( &buffer, FLD_FLOAT1, 3.14) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( casual_field_remove_occurrence( buffer, FLD_LONG1, 0) == CASUAL_FIELD_SUCCESS);

         long id = CASUAL_FIELD_NO_ID;
         long occurrence;

         EXPECT_TRUE( casual_field_next( buffer, &id, &occurrence) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( id == FLD_SHORT1) << id;
         EXPECT_TRUE( occurrence == 0);

         EXPECT_TRUE( casual_field_next( buffer, &id, &occurrence) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( id == FLD_FLOAT1) << id;
         EXPECT_TRUE( occurrence == 0);

         EXPECT_TRUE( casual_field_next( buffer, &id, &occurrence) == CASUAL_FIELD_OUT_OF_BOUNDS);

         tpfree( buffer);
      }

      TEST( casual_field_buffer, get_pod_size__expecting_correct_sizes)
      {

         long size = 0;
         EXPECT_FALSE( casual_field_plain_type_host_size( CASUAL_FIELD_SHORT, &size));
         EXPECT_TRUE( size == sizeof( short));
         EXPECT_FALSE( casual_field_plain_type_host_size( CASUAL_FIELD_LONG, &size));
         EXPECT_TRUE( size == sizeof( long));
         EXPECT_FALSE( casual_field_plain_type_host_size( CASUAL_FIELD_CHAR, &size));
         EXPECT_TRUE( size == sizeof( char));
         EXPECT_FALSE( casual_field_plain_type_host_size( CASUAL_FIELD_FLOAT, &size));
         EXPECT_TRUE( size == sizeof( float));
         EXPECT_FALSE( casual_field_plain_type_host_size( CASUAL_FIELD_DOUBLE, &size));
         EXPECT_TRUE( size == sizeof( double));
         EXPECT_TRUE( casual_field_plain_type_host_size( CASUAL_FIELD_STRING, &size) == CASUAL_FIELD_INVALID_ARGUMENT);
         EXPECT_TRUE( casual_field_plain_type_host_size( CASUAL_FIELD_BINARY, &size) == CASUAL_FIELD_INVALID_ARGUMENT);
         EXPECT_TRUE( casual_field_plain_type_host_size( CASUAL_FIELD_SHORT, nullptr) == CASUAL_FIELD_INVALID_ARGUMENT);

      }


      TEST( casual_field_buffer, add_values_as_void_and_get_as_type__expecting_success)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 512);
         ASSERT_TRUE( buffer != nullptr);

         {
            short source = 123;
            EXPECT_FALSE( casual_field_add_value( &buffer, FLD_SHORT1, &source, 0));
            short target{};
            EXPECT_FALSE( casual_field_get_short( buffer, FLD_SHORT1, 0, &target));
            EXPECT_TRUE( source == target);
         }

         {
            long source = 123456;
            EXPECT_FALSE( casual_field_add_value( &buffer, FLD_LONG1, &source, 0));
            long target{};
            EXPECT_FALSE( casual_field_get_long( buffer, FLD_LONG1, 0, &target));
            EXPECT_TRUE( source == target);
         }

         {
            char source = 'a';
            EXPECT_FALSE( casual_field_add_value( &buffer, FLD_CHAR1, &source, 0));
            char target{};
            EXPECT_FALSE( casual_field_get_char( buffer, FLD_CHAR1, 0, &target));
            EXPECT_TRUE( source == target);
         }

         {
            float source = 3.14;
            EXPECT_FALSE( casual_field_add_value( &buffer, FLD_FLOAT1, &source, 0));
            float target{};
            EXPECT_FALSE( casual_field_get_float( buffer, FLD_FLOAT1, 0, &target));
            EXPECT_TRUE( source == target);
         }

         {
            double source = 123.456;
            EXPECT_FALSE( casual_field_add_value( &buffer, FLD_DOUBLE1, &source, 0));
            double target{};
            EXPECT_FALSE( casual_field_get_double( buffer, FLD_DOUBLE1, 0, &target));
            EXPECT_TRUE( source == target);
         }

         {
            const char* source = "Hello World!";
            EXPECT_FALSE( casual_field_add_value( &buffer, FLD_STRING1, source, 0));
            const char* target{};
            EXPECT_FALSE( casual_field_get_string( buffer, FLD_STRING1, 0, &target));
            EXPECT_STREQ( source, target);
         }

         {
            const char source[] = "Hello World!";
            EXPECT_FALSE( casual_field_add_value( &buffer, FLD_BINARY1, source, sizeof(source)));
            const char* target{};
            long count{};
            EXPECT_FALSE( casual_field_get_binary( buffer, FLD_BINARY1, 0, &target, &count));
            EXPECT_STREQ( source, target);
            EXPECT_TRUE( count == sizeof(source));
         }

         tpfree( buffer);
      }

      TEST( casual_field_buffer, add_values_as_type_and_get_as_void__expecting_success)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 512);
         ASSERT_TRUE( buffer != nullptr);

         {
            short source = 123;
            EXPECT_FALSE( casual_field_add_short( &buffer, FLD_SHORT1, source));
            short target{};
            EXPECT_FALSE( casual_field_get_value( buffer, FLD_SHORT1, 0, &target, nullptr));
            EXPECT_TRUE( source == target);
         }

         {
            long source = 123456;
            EXPECT_FALSE( casual_field_add_long( &buffer, FLD_LONG1, source));
            long target{};
            EXPECT_FALSE( casual_field_get_value( buffer, FLD_LONG1, 0, &target, nullptr));
            EXPECT_TRUE( source == target);
         }

         {
            char source = 'a';
            EXPECT_FALSE( casual_field_add_char( &buffer, FLD_CHAR1, source));
            char target{};
            EXPECT_FALSE( casual_field_get_value( buffer, FLD_CHAR1, 0, &target, nullptr));
            EXPECT_TRUE( source == target);
         }

         {
            float source = 3.14;
            EXPECT_FALSE( casual_field_add_float( &buffer, FLD_FLOAT1, source));
            float target{};
            EXPECT_FALSE( casual_field_get_value( buffer, FLD_FLOAT1, 0, &target, nullptr));
            EXPECT_TRUE( source == target);
         }

         {
            double source = 123.456;
            EXPECT_FALSE( casual_field_add_double( &buffer, FLD_DOUBLE1, source));
            double target{};
            EXPECT_FALSE( casual_field_get_value( buffer, FLD_DOUBLE1, 0, &target, nullptr));
            EXPECT_TRUE( source == target);
         }

         {
            const char source[] = "Hello World!";
            EXPECT_FALSE( casual_field_add_string( &buffer, FLD_STRING1, source));
            char target[64];
            long count = sizeof( target);
            EXPECT_FALSE( casual_field_get_value( buffer, FLD_STRING1, 0, target, &count));
            EXPECT_STREQ( source, target);
            EXPECT_TRUE( count == sizeof( source));
         }

         {
            const char source[] = "Hello World!";
            EXPECT_FALSE( casual_field_add_binary( &buffer, FLD_BINARY1, source, sizeof( source)));
            char target[64];
            long count = sizeof( target);
            EXPECT_FALSE( casual_field_get_value( buffer, FLD_STRING1, 0, target, &count));
            EXPECT_STREQ( source, target);
            EXPECT_TRUE( count == sizeof( source));
         }

         {
            const char source[] = "Hello World!";
            EXPECT_FALSE( casual_field_add_string( &buffer, FLD_STRING1, source));
            char target[4];
            long count = sizeof( target);
            EXPECT_TRUE( casual_field_get_value( buffer, FLD_STRING1, 0, target, &count) == CASUAL_FIELD_INVALID_ARGUMENT);
         }



         tpfree( buffer);
      }


      TEST( casual_field_buffer, add_short_and_then_update_it__expecting_updated_value_on_get)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 512);
         ASSERT_TRUE( buffer != nullptr);

         {
            {
               short source = 123;
               EXPECT_FALSE( casual_field_add_short( &buffer, FLD_SHORT1, source));
               short target{};
               EXPECT_FALSE( casual_field_get_short( buffer, FLD_SHORT1, 0, &target));
               EXPECT_TRUE( source == target);
            }

            {
               short source = 321;
               EXPECT_FALSE( casual_field_set_short( &buffer, FLD_SHORT1, 0, source));
               short target{};
               EXPECT_FALSE( casual_field_get_short( buffer, FLD_SHORT1, 0, &target));
               EXPECT_TRUE( source == target);
            }
         }

         tpfree( buffer);
      }

      TEST( casual_field_buffer, add_string_of_certain_size_and_update_it__expecting_equality)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 512);
         ASSERT_TRUE( buffer != nullptr);

         {
            {
               const char source[] = "A normal string";
               EXPECT_FALSE( casual_field_add_string( &buffer, FLD_STRING1, source));
               const char* target{};
               EXPECT_FALSE( casual_field_get_string( buffer, FLD_STRING1, 0, &target));
               EXPECT_STREQ( source, target);
            }

            const char other[] = "An other string";
            EXPECT_FALSE( casual_field_add_string( &buffer, FLD_STRING1, other));

            {
               const char* target{};
               EXPECT_FALSE( casual_field_get_string( buffer, FLD_STRING1, 1, &target));
               EXPECT_STREQ( other, target);
            }

            {
               const char source[] = "A looooooooooooooong string";
               EXPECT_FALSE( casual_field_set_string( &buffer, FLD_STRING1, 0, source));
               const char* target{};
               EXPECT_FALSE( casual_field_get_string( buffer, FLD_STRING1, 0, &target));
               EXPECT_STREQ( source, target);
            }


            {
               const char* target{};
               EXPECT_FALSE( casual_field_get_string( buffer, FLD_STRING1, 1, &target));
               EXPECT_STREQ( other, target);
            }


            {
               const char source[] = "A tiny string";
               EXPECT_FALSE( casual_field_set_string( &buffer, FLD_STRING1, 0, source));
               const char* target{};
               EXPECT_FALSE( casual_field_get_string( buffer, FLD_STRING1, 0, &target));
               EXPECT_STREQ( source, target);
            }

            {
               const char* target{};
               EXPECT_FALSE( casual_field_get_string( buffer, FLD_STRING1, 1, &target));
               EXPECT_STREQ( other, target);
            }


         }

         tpfree( buffer);
      }


      TEST( casual_field_buffer, count_occurrences__expecting_correct_numbers)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 512);
         ASSERT_TRUE( buffer != nullptr);

         EXPECT_FALSE( casual_field_add_string( &buffer, FLD_STRING1, "First string 1"));
         EXPECT_FALSE( casual_field_add_string( &buffer, FLD_STRING2, "First string 2"));
         EXPECT_FALSE( casual_field_add_string( &buffer, FLD_STRING1, "Other string 1"));

         {
            long occurrences{};
            EXPECT_FALSE( casual_field_occurrences_of_id( buffer, FLD_STRING1, &occurrences));
            EXPECT_TRUE( occurrences == 2);
         }

         {
            long occurrences{};
            EXPECT_FALSE( casual_field_occurrences_in_buffer( buffer, &occurrences));
            EXPECT_TRUE( occurrences == 3);
         }

         tpfree( buffer);
      }

      TEST( casual_field_buffer, serialize_buffer__expecting_success)
      {
         auto source = tpalloc( CASUAL_FIELD, "", 512);
         ASSERT_TRUE( source != nullptr);

         EXPECT_FALSE( casual_field_add_string( &source, FLD_STRING1, "Casual"));

         long memory_size{};

         ASSERT_FALSE( casual_field_explore_buffer( source, nullptr, &memory_size));

         std::vector<char> memory( source, source + memory_size);

         tpfree( source);


         auto target = tpalloc( CASUAL_FIELD, "", memory_size);

         ASSERT_FALSE( casual_field_copy_memory( &target, memory.data(), memory.size()));


         const char* string = nullptr;

         ASSERT_FALSE( casual_field_get_string( target, FLD_STRING1, 0, &string));

         EXPECT_STREQ( string, "Casual");


         tpfree( target);

      }

      TEST( casual_field_buffer, DISABLED_performance__expecting_good_enough_speed)
      {
         for( long idx = 0; idx < 100000; ++idx)
         {
            auto buffer = tpalloc( CASUAL_FIELD, "", 5120);
            ASSERT_TRUE( buffer != nullptr);

            const long iterations = 10;

            for( long idx = 0; idx < iterations; ++idx)
            {
               ASSERT_TRUE( casual_field_add_char( &buffer, FLD_CHAR1, 'a') == CASUAL_FIELD_SUCCESS);
               ASSERT_TRUE( casual_field_add_short( &buffer, FLD_SHORT1, 123) == CASUAL_FIELD_SUCCESS);
               ASSERT_TRUE( casual_field_add_long( &buffer, FLD_LONG1, 654321) == CASUAL_FIELD_SUCCESS);
               ASSERT_TRUE( casual_field_add_float( &buffer, FLD_FLOAT1, 3.14) == CASUAL_FIELD_SUCCESS);
               ASSERT_TRUE( casual_field_add_double( &buffer, FLD_DOUBLE1, 987.654) == CASUAL_FIELD_SUCCESS);
               ASSERT_TRUE( casual_field_add_string( &buffer, FLD_STRING1, "Hello!") == CASUAL_FIELD_SUCCESS);
               ASSERT_TRUE( casual_field_add_binary( &buffer, FLD_BINARY1, "Some Data", 9) == CASUAL_FIELD_SUCCESS);
            }

            for( long idx = 0; idx < iterations; ++idx)
            {
               char character;
               ASSERT_TRUE( casual_field_get_char( buffer, FLD_CHAR1, idx, &character) == CASUAL_FIELD_SUCCESS);

               short short_integer;
               ASSERT_TRUE( casual_field_get_short( buffer, FLD_SHORT1, idx, &short_integer) == CASUAL_FIELD_SUCCESS);

               long long_integer;
               ASSERT_TRUE( casual_field_get_long( buffer, FLD_LONG1, idx, &long_integer) == CASUAL_FIELD_SUCCESS);

               float short_decimal;
               ASSERT_TRUE( casual_field_get_float( buffer, FLD_FLOAT1, idx, &short_decimal) == CASUAL_FIELD_SUCCESS);

               double long_decimal;
               ASSERT_TRUE( casual_field_get_double( buffer, FLD_DOUBLE1, idx, &long_decimal) == CASUAL_FIELD_SUCCESS);

               const char* string;
               ASSERT_TRUE( casual_field_get_string( buffer, FLD_STRING1, idx, &string) == CASUAL_FIELD_SUCCESS);

               const char* binary;
               long size;
               ASSERT_TRUE( casual_field_get_binary( buffer, FLD_BINARY1, idx, &binary, &size) == CASUAL_FIELD_SUCCESS);

            }

            tpfree( buffer);
         }
      }

   }

}

