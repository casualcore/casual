//
// test_field_buffer.cpp
//
//  Created on: 12 nov 2013
//      Author: Kristone
//


#include <gtest/gtest.h>

#include "buffer/field.h"
#include "common/environment.h"

#include "xatmi.h"


#include <string>

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

      TEST( casual_field_buffer, use_with_invalid_buffer__expecting_invalid_buffer)
      {
         char invalid[] = "some invalid buffer";

         EXPECT_TRUE( CasualFieldExploreBuffer( nullptr, nullptr, nullptr) == CASUAL_FIELD_INVALID_BUFFER);

         EXPECT_TRUE( CasualFieldExploreBuffer( invalid, nullptr, nullptr) == CASUAL_FIELD_INVALID_BUFFER);

         EXPECT_TRUE( CasualFieldAddChar( invalid, FLD_CHAR1, 'a') == CASUAL_FIELD_INVALID_BUFFER);
         EXPECT_TRUE( CasualFieldAddShort( invalid, FLD_SHORT1, 123) == CASUAL_FIELD_INVALID_BUFFER);
         EXPECT_TRUE( CasualFieldAddLong( invalid, FLD_LONG1, 123456) == CASUAL_FIELD_INVALID_BUFFER);
         EXPECT_TRUE( CasualFieldAddFloat( invalid, FLD_FLOAT1, 123.456) == CASUAL_FIELD_INVALID_BUFFER);
         EXPECT_TRUE( CasualFieldAddDouble( invalid, FLD_DOUBLE1, 123456.789) == CASUAL_FIELD_INVALID_BUFFER);
         EXPECT_TRUE( CasualFieldAddString( invalid, FLD_STRING1, "Casual rules!") == CASUAL_FIELD_INVALID_BUFFER);
         EXPECT_TRUE( CasualFieldAddBinary( invalid, FLD_BINARY1, "!#¤%&", 5) == CASUAL_FIELD_INVALID_BUFFER);

         EXPECT_TRUE( CasualFieldGetChar( invalid, FLD_CHAR1, 0, nullptr) == CASUAL_FIELD_INVALID_BUFFER);
         EXPECT_TRUE( CasualFieldGetShort( invalid, FLD_SHORT1, 0, nullptr) == CASUAL_FIELD_INVALID_BUFFER);
         EXPECT_TRUE( CasualFieldGetLong( invalid, FLD_LONG1, 0, nullptr) == CASUAL_FIELD_INVALID_BUFFER);
         EXPECT_TRUE( CasualFieldGetFloat( invalid, FLD_FLOAT1, 0, nullptr) == CASUAL_FIELD_INVALID_BUFFER);
         EXPECT_TRUE( CasualFieldGetDouble( invalid, FLD_DOUBLE1, 0, nullptr) == CASUAL_FIELD_INVALID_BUFFER);
         EXPECT_TRUE( CasualFieldGetString( invalid, FLD_STRING1, 0, nullptr) == CASUAL_FIELD_INVALID_BUFFER);
         EXPECT_TRUE( CasualFieldGetBinary( invalid, FLD_BINARY1, 0, nullptr, nullptr) == CASUAL_FIELD_INVALID_BUFFER);

         EXPECT_TRUE( CasualFieldExploreValue( invalid, FLD_CHAR1, 0, nullptr) == CASUAL_FIELD_INVALID_BUFFER);

         EXPECT_TRUE( CasualFieldRemoveAll( invalid) == CASUAL_FIELD_INVALID_BUFFER);

         EXPECT_TRUE( CasualFieldRemoveId( invalid, FLD_CHAR1) == CASUAL_FIELD_INVALID_BUFFER);

         EXPECT_TRUE( CasualFieldRemoveOccurrence( invalid, FLD_CHAR1, 0) == CASUAL_FIELD_INVALID_BUFFER);

         auto buffer = tpalloc( CASUAL_FIELD, "", 0);
         ASSERT_TRUE( buffer != nullptr);

         EXPECT_TRUE( CasualFieldCopyBuffer( invalid, buffer) == CASUAL_FIELD_INVALID_BUFFER);
         EXPECT_TRUE( CasualFieldCopyBuffer( buffer, invalid) == CASUAL_FIELD_INVALID_BUFFER);

         tpfree( buffer);

         long id = CASUAL_FIELD_NO_ID;
         long occurrence;
         EXPECT_TRUE( CasualFieldNext( invalid, &id, &occurrence) == CASUAL_FIELD_INVALID_BUFFER);

      }

      TEST( casual_field_buffer, use_with_wrong_buffer__expecting_invalid_buffer)
      {
         auto buffer = tpalloc( "X_OCTET", "binary", 128);
         ASSERT_TRUE( buffer != nullptr);

         EXPECT_TRUE( CasualFieldAddChar( buffer, FLD_CHAR1, 'a') == CASUAL_FIELD_INVALID_BUFFER);

         tpfree( buffer);
      }

      TEST( casual_field_buffer, add_binary_data_with_negative_size__expecting_invalid_argument)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 0);
         ASSERT_TRUE( buffer != nullptr);

         EXPECT_TRUE( CasualFieldAddBinary( buffer, FLD_BINARY1, "some data", -123) == CASUAL_FIELD_INVALID_ARGUMENT);

         tpfree( buffer);
      }

      TEST( casual_field_buffer, allocate_with_zero_size__expecting_success)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 0);
         ASSERT_TRUE( buffer != nullptr);

         long used = 0;
         EXPECT_TRUE( CasualFieldExploreBuffer( buffer, nullptr, &used) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( used == 0);
         tpfree( buffer);
      }

      TEST( casual_field_buffer, reallocate_with_larger_size__expecting_success)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 64);
         ASSERT_TRUE( buffer != nullptr);

         long initial_size = 0;
         EXPECT_TRUE( CasualFieldExploreBuffer( buffer, &initial_size, nullptr) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( initial_size == 64);

         buffer = tprealloc( buffer, 128);
         ASSERT_TRUE( buffer != nullptr);

         long updated_size = 0;
         EXPECT_TRUE( CasualFieldExploreBuffer( buffer, &updated_size, nullptr) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( updated_size == 128);

         tpfree( buffer);
      }


      TEST( casual_field_buffer, reallocate_with_smaller_size__expecting_success)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 128);
         ASSERT_TRUE( buffer != nullptr);

         long initial_size = 0;
         EXPECT_TRUE( CasualFieldExploreBuffer( buffer, &initial_size, nullptr) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( initial_size == 128);

         EXPECT_TRUE( CasualFieldAddString( buffer, FLD_STRING1, "goin' casual today!") == CASUAL_FIELD_SUCCESS);

         buffer = tprealloc( buffer, 64);
         ASSERT_TRUE( buffer != nullptr);

         long updated_size = 0;
         EXPECT_TRUE( CasualFieldExploreBuffer( buffer, &updated_size, nullptr) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( updated_size == 64);

         buffer = tprealloc( buffer, 0);
         ASSERT_TRUE( buffer != nullptr);

         long final_size = 0;
         long final_used = 0;
         EXPECT_TRUE( CasualFieldExploreBuffer( buffer, &final_size, &final_used) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( final_size < 64);
         EXPECT_TRUE( final_size > 0);
         EXPECT_TRUE( final_size == final_used);

         tpfree( buffer);
      }

      TEST( casual_field_buffer, allocate_with_small_size_and_write_much___expecting_no_space)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 64);
         ASSERT_TRUE( buffer != nullptr);

         const char string[] = "A long string that is longer than what would fit in this small buffer";

         EXPECT_TRUE( CasualFieldAddString( buffer, FLD_STRING1, string) == CASUAL_FIELD_NO_SPACE);

         tpfree( buffer);
      }


      TEST( casual_field_buffer, add_and_get__expecting_success)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 512);
         ASSERT_TRUE( buffer != nullptr);

         EXPECT_TRUE( CasualFieldAddChar( buffer, FLD_CHAR1, 'a') == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( CasualFieldAddShort( buffer, FLD_SHORT1, 123) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( CasualFieldAddLong( buffer, FLD_LONG1, 654321) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( CasualFieldAddFloat( buffer, FLD_FLOAT1, 3.14) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( CasualFieldAddDouble( buffer, FLD_DOUBLE1, 987.654) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( CasualFieldAddString( buffer, FLD_STRING1, "Hello!") == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( CasualFieldAddBinary( buffer, FLD_BINARY1, "Some Data", 9) == CASUAL_FIELD_SUCCESS);

         {
            char character;
            EXPECT_TRUE( CasualFieldGetChar( buffer, FLD_CHAR1, 0, &character) == CASUAL_FIELD_SUCCESS);
            EXPECT_TRUE( character == 'a');
         }

         {
            short short_integer;
            EXPECT_TRUE( CasualFieldGetShort( buffer, FLD_SHORT1, 0, &short_integer) == CASUAL_FIELD_SUCCESS);
            EXPECT_TRUE( short_integer == 123);
         }

         {
            long long_integer;
            EXPECT_TRUE( CasualFieldGetLong( buffer, FLD_LONG1, 0, &long_integer) == CASUAL_FIELD_SUCCESS);
            EXPECT_TRUE( long_integer == 654321);
         }

         {
            float short_decimal;
            EXPECT_TRUE( CasualFieldGetFloat( buffer, FLD_FLOAT1, 0, &short_decimal) == CASUAL_FIELD_SUCCESS);
            EXPECT_TRUE( short_decimal > 3.1 && short_decimal < 3.2);
         }

         {
            double long_decimal;
            EXPECT_TRUE( CasualFieldGetDouble( buffer, FLD_DOUBLE1, 0, &long_decimal) == CASUAL_FIELD_SUCCESS);
            EXPECT_TRUE( long_decimal > 987.6 && long_decimal < 987.7);
         }

         {
            const char* string;
            EXPECT_TRUE( CasualFieldGetString( buffer, FLD_STRING1, 0, &string) == CASUAL_FIELD_SUCCESS);
            EXPECT_STREQ( string, "Hello!") << string;
         }

         {
            const char* binary;
            long size;
            EXPECT_TRUE( CasualFieldGetBinary( buffer, FLD_BINARY1, 0, &binary, &size) == CASUAL_FIELD_SUCCESS);
            EXPECT_TRUE( std::string( binary, size) == "Some Data") << std::string( binary, size);
            EXPECT_TRUE( size == 9);
         }

         tpfree( buffer);
      }


      TEST( casual_field_buffer, add_and_get_several_occurrences__expecting_success)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 512);
         ASSERT_TRUE( buffer != nullptr);

         EXPECT_TRUE( CasualFieldAddShort( buffer, FLD_SHORT1, 123) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( CasualFieldAddString( buffer, FLD_STRING1, "Hello!") == CASUAL_FIELD_SUCCESS);

         EXPECT_TRUE( CasualFieldAddShort( buffer, FLD_SHORT1, 321) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( CasualFieldAddString( buffer, FLD_STRING1, "!olleH") == CASUAL_FIELD_SUCCESS);



         short short_integer;
         EXPECT_TRUE( CasualFieldGetShort( buffer, FLD_SHORT1, 0, &short_integer) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( short_integer == 123);

         EXPECT_TRUE( CasualFieldGetShort( buffer, FLD_SHORT1, 1, &short_integer) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( short_integer == 321);


         const char* string;
         EXPECT_TRUE( CasualFieldGetString( buffer, FLD_STRING1, 0, &string) == CASUAL_FIELD_SUCCESS);
         EXPECT_STREQ( string, "Hello!") << string;

         EXPECT_TRUE( CasualFieldGetString( buffer, FLD_STRING1, 1, &string) == CASUAL_FIELD_SUCCESS);
         EXPECT_STREQ( string, "!olleH") << string;

         tpfree( buffer);
      }


      TEST( casual_field_buffer, add_one_and_get_two_occurrences__expecting_no_occurrence)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 512);
         ASSERT_TRUE( buffer != nullptr);

         EXPECT_TRUE( CasualFieldAddShort( buffer, FLD_SHORT1, 123) == CASUAL_FIELD_SUCCESS);

         short short_integer;
         EXPECT_TRUE( CasualFieldGetShort( buffer, FLD_SHORT1, 0, &short_integer) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( CasualFieldGetShort( buffer, FLD_SHORT1, 1, &short_integer) == CASUAL_FIELD_NO_OCCURRENCE);

         tpfree( buffer);
      }

      TEST( casual_field_buffer, add_short_with_id_of_long__expecting_invalid_id)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 512);
         ASSERT_TRUE( buffer != nullptr);

         EXPECT_TRUE( CasualFieldAddShort( buffer, FLD_LONG1, 123) == CASUAL_FIELD_INVALID_ID);

         tpfree( buffer);
      }

      TEST( casual_field_buffer, add_short_with_invalid_id__expecting_invalid_id)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 512);
         ASSERT_TRUE( buffer != nullptr);

         EXPECT_TRUE( CasualFieldAddShort( buffer, -1, 123) == CASUAL_FIELD_INVALID_ID);
         EXPECT_TRUE( CasualFieldAddShort( buffer, 0, 123) == CASUAL_FIELD_INVALID_ID);

         tpfree( buffer);
      }


      TEST( casual_field_buffer, get_type_from_id__expecting_success)
      {
         int type;
         EXPECT_TRUE( CasualFieldTypeOfId( FLD_LONG1, &type) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( type == CASUAL_FIELD_LONG);
      }

      TEST( casual_field_buffer, type_from_invalid_id__expecting_invalid_id)
      {
         int type;
         EXPECT_TRUE( CasualFieldTypeOfId( 0, &type) == CASUAL_FIELD_INVALID_ID);
         EXPECT_TRUE( CasualFieldTypeOfId( -1, &type) == CASUAL_FIELD_INVALID_ID);
      }

      TEST( casual_field_buffer, remove_occurrence_with_invalid_id__expecting_invalid_id)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 512);
         ASSERT_TRUE( buffer != nullptr);

         EXPECT_TRUE( CasualFieldRemoveOccurrence( buffer, 0, 0) == CASUAL_FIELD_INVALID_ID);
         EXPECT_TRUE( CasualFieldRemoveOccurrence( buffer, -1, 0) == CASUAL_FIELD_INVALID_ID);

         tpfree( buffer);
      }


      TEST( casual_field_buffer, remove_occurrence_with_no_occurrence__expecting_no_occurrence)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 512);
         ASSERT_TRUE( buffer != nullptr);

         EXPECT_TRUE( CasualFieldExploreValue( buffer, FLD_LONG1, 0, nullptr) == CASUAL_FIELD_NO_OCCURRENCE);
         EXPECT_TRUE( CasualFieldRemoveOccurrence( buffer, FLD_LONG1, 0) == CASUAL_FIELD_NO_OCCURRENCE);

         tpfree( buffer);
      }

      TEST( casual_field_buffer, add_and_remove_occurrence__expecting_success)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 512);
         ASSERT_TRUE( buffer != nullptr);

         EXPECT_TRUE( CasualFieldAddShort( buffer, FLD_SHORT1, 123) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( CasualFieldRemoveOccurrence( buffer, FLD_SHORT1, 0) == CASUAL_FIELD_SUCCESS);

         tpfree( buffer);
      }

      TEST( casual_field_buffer, add_two_and_remove_first_occurrence_and_get_second_as_first__expecting_success)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 512);
         ASSERT_TRUE( buffer != nullptr);

         EXPECT_TRUE( CasualFieldAddShort( buffer, FLD_SHORT1, 123) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( CasualFieldAddShort( buffer, FLD_SHORT1, 321) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( CasualFieldRemoveOccurrence( buffer, FLD_SHORT1, 0) == CASUAL_FIELD_SUCCESS);
         short short_integer;
         EXPECT_TRUE( CasualFieldGetShort( buffer, FLD_SHORT1, 0, &short_integer) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( short_integer == 321);

         tpfree( buffer);
      }

      TEST( casual_field_buffer, add_twp_and_remove_second_occurrence_and_first_second_as_first__expecting_success)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 512);
         ASSERT_TRUE( buffer != nullptr);

         EXPECT_TRUE( CasualFieldAddShort( buffer, FLD_SHORT1, 123) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( CasualFieldAddShort( buffer, FLD_SHORT1, 321) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( CasualFieldRemoveOccurrence( buffer, FLD_SHORT1, 1) == CASUAL_FIELD_SUCCESS);
         short short_integer;
         EXPECT_TRUE( CasualFieldGetShort( buffer, FLD_SHORT1, 0, &short_integer) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( short_integer == 123);

         tpfree( buffer);
      }


      TEST( casual_field_buffer, remove_all_occurrences_and_check_existence__expecting_no_occurrence)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 512);
         ASSERT_TRUE( buffer != nullptr);

         EXPECT_TRUE( CasualFieldAddShort( buffer, FLD_SHORT1, 123) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( CasualFieldAddShort( buffer, FLD_SHORT1, 321) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( CasualFieldExploreValue( buffer, FLD_SHORT1, 0, nullptr) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( CasualFieldRemoveId( buffer, FLD_SHORT1) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( CasualFieldExploreValue( buffer, FLD_SHORT1, 0, nullptr) == CASUAL_FIELD_NO_OCCURRENCE);

         tpfree( buffer);
      }

      TEST( casual_field_buffer, add_to_source_and_copy_buffer_and_get_from_target__expecting_success)
      {
         auto source = tpalloc( CASUAL_FIELD, "", 64);
         ASSERT_TRUE( source != nullptr);

         auto target = tpalloc( CASUAL_FIELD, "", 128);
         ASSERT_TRUE( target != nullptr);


         EXPECT_TRUE( CasualFieldAddShort( source, FLD_SHORT1, 123) == CASUAL_FIELD_SUCCESS);
         ASSERT_TRUE( CasualFieldCopyBuffer( target, source) == CASUAL_FIELD_SUCCESS);

         {
            long size = 0;
            CasualFieldExploreBuffer( source, &size, nullptr);
            ASSERT_TRUE( size == 64);
         }

         {
            long size = 0;
            CasualFieldExploreBuffer( target, &size, nullptr);
            ASSERT_TRUE( size == 128);
         }

         short short_integer;
         EXPECT_TRUE( CasualFieldGetShort( target, FLD_SHORT1, 0, &short_integer) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( short_integer == 123);


         tpfree( source);
         tpfree( target);
      }


      TEST( casual_field_buffer, test_some_iteration)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 128);

         ASSERT_TRUE( buffer != nullptr);

         ASSERT_TRUE( CasualFieldAddShort( buffer, FLD_SHORT1, 123) == CASUAL_FIELD_SUCCESS);
         ASSERT_TRUE( CasualFieldAddLong( buffer, FLD_LONG1, 123456) == CASUAL_FIELD_SUCCESS);
         ASSERT_TRUE( CasualFieldAddLong( buffer, FLD_LONG1, 654321) == CASUAL_FIELD_SUCCESS);
         ASSERT_TRUE( CasualFieldAddShort( buffer, FLD_SHORT1, 456) == CASUAL_FIELD_SUCCESS);
         ASSERT_TRUE( CasualFieldAddLong( buffer, FLD_LONG1, 987654321) == CASUAL_FIELD_SUCCESS);

         long id = CASUAL_FIELD_NO_ID;
         long occurrence = 0;

         EXPECT_TRUE( CasualFieldNext( buffer, &id, &occurrence) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( id == FLD_SHORT1);
         EXPECT_TRUE( occurrence == 0);
         EXPECT_TRUE( CasualFieldNext( buffer, &id, &occurrence) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( id == FLD_LONG1);
         EXPECT_TRUE( occurrence == 0);
         EXPECT_TRUE( CasualFieldNext( buffer, &id, &occurrence) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( id == FLD_LONG1);
         EXPECT_TRUE( occurrence == 1);
         EXPECT_TRUE( CasualFieldNext( buffer, &id, &occurrence) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( id == FLD_SHORT1);
         EXPECT_TRUE( occurrence == 1);
         EXPECT_TRUE( CasualFieldNext( buffer, &id, &occurrence) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( id == FLD_LONG1);
         EXPECT_TRUE( occurrence == 2);
         EXPECT_TRUE( CasualFieldNext( buffer, &id, &occurrence) == CASUAL_FIELD_NO_OCCURRENCE);

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
               casual::common::environment::variable::set( "CASUAL_FIELD_TABLE", "CASUAL_FIELD_TABLE.json");
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
         ASSERT_TRUE( CasualFieldNameOfId( FLD_SHORT1, &name) == CASUAL_FIELD_SUCCESS);
         EXPECT_STREQ( name, "FLD_SHORT1");
      }

      TEST_F( casual_field_buffer_repository, get_name_from_id_to_null__expecting_success_and_no_crasch)
      {
         ASSERT_TRUE( CasualFieldNameOfId( FLD_SHORT1, nullptr) == CASUAL_FIELD_SUCCESS);
      }

      TEST_F( casual_field_buffer_repository, get_name_from_id_from_group_two__expecting_success)
      {
         const char* name;
         ASSERT_TRUE( CasualFieldNameOfId( FLD_DOUBLE2, &name) == CASUAL_FIELD_SUCCESS);
         EXPECT_STREQ( name, "FLD_DOUBLE2");
      }

      TEST_F( casual_field_buffer_repository, get_id_from_name_from_group_one__expecting_success)
      {
         long id;
         ASSERT_TRUE( CasualFieldIdOfName( "FLD_SHORT2", &id) == CASUAL_FIELD_SUCCESS);
         EXPECT_EQ( id, FLD_SHORT2);
      }

      TEST_F( casual_field_buffer_repository, get_id_from_name_to_null__expecting_success_and_no_crasch)
      {
         ASSERT_TRUE( CasualFieldIdOfName( "FLD_SHORT2", nullptr) == CASUAL_FIELD_SUCCESS);
      }

      TEST_F( casual_field_buffer_repository, get_id_from_name_from_group_two__expecting_success)
      {
         long id;
         ASSERT_TRUE( CasualFieldIdOfName( "FLD_DOUBLE3", &id) == CASUAL_FIELD_SUCCESS);
         EXPECT_EQ( id, FLD_DOUBLE3);
      }

      TEST_F( casual_field_buffer_repository, get_name_from_non_existing_id__expecting_unkown_id)
      {
         const char* name;
         ASSERT_TRUE( CasualFieldNameOfId( 666, &name) == CASUAL_FIELD_UNKNOWN_ID);
      }

      TEST_F( casual_field_buffer_repository, get_id_from_non_existing_name__expecting_unknown_id)
      {
         long id;
         ASSERT_TRUE( CasualFieldIdOfName( "NON_EXISTING_NAME", &id) == CASUAL_FIELD_UNKNOWN_ID);
      }

      TEST_F( casual_field_buffer_repository, get_name_from_invalid_id__expecting_invalid_id)
      {
         const char* name;
         ASSERT_TRUE( CasualFieldNameOfId( -666, &name) == CASUAL_FIELD_INVALID_ID);
      }

      TEST_F( casual_field_buffer_repository, get_id_from_invalid_name__expecting_invalid_id)
      {
         long id;
         ASSERT_TRUE( CasualFieldIdOfName( nullptr, &id) == CASUAL_FIELD_INVALID_ID);
      }

      TEST_F( casual_field_buffer_repository, get_name_of_type__expecting_success)
      {
         const char* name = nullptr;
         EXPECT_TRUE( CasualFieldNameOfType( CASUAL_FIELD_SHORT, &name) == CASUAL_FIELD_SUCCESS);
         EXPECT_STREQ( name, "short");
         EXPECT_TRUE( CasualFieldNameOfType( CASUAL_FIELD_LONG, &name) == CASUAL_FIELD_SUCCESS);
         EXPECT_STREQ( name, "long");
         EXPECT_TRUE( CasualFieldNameOfType( CASUAL_FIELD_CHAR, &name) == CASUAL_FIELD_SUCCESS);
         EXPECT_STREQ( name, "char");
         EXPECT_TRUE( CasualFieldNameOfType( CASUAL_FIELD_FLOAT, &name) == CASUAL_FIELD_SUCCESS);
         EXPECT_STREQ( name, "float");
         EXPECT_TRUE( CasualFieldNameOfType( CASUAL_FIELD_DOUBLE, &name) == CASUAL_FIELD_SUCCESS);
         EXPECT_STREQ( name, "double");
         EXPECT_TRUE( CasualFieldNameOfType( CASUAL_FIELD_STRING, &name) == CASUAL_FIELD_SUCCESS);
         EXPECT_STREQ( name, "string");
         EXPECT_TRUE( CasualFieldNameOfType( CASUAL_FIELD_BINARY, &name) == CASUAL_FIELD_SUCCESS);
         EXPECT_STREQ( name, "binary");
      }

      TEST_F( casual_field_buffer_repository, get_name_of_invalid_type__expecting_invalid_type)
      {
         const char* name = nullptr;
         EXPECT_TRUE( CasualFieldNameOfType( 666, &name) == CASUAL_FIELD_INVALID_TYPE);
      }

      TEST_F( casual_field_buffer_repository, get_name_of_type_to_null__expecting_success_and_no_crasch)
      {
         EXPECT_TRUE( CasualFieldNameOfType( CASUAL_FIELD_STRING, nullptr) == CASUAL_FIELD_SUCCESS);
      }

      TEST_F( casual_field_buffer_repository, get_type_of_name__expecting_success)
      {
         int type = -1;
         EXPECT_TRUE( CasualFieldTypeOfName( "short", &type) == CASUAL_FIELD_SUCCESS);
         EXPECT_EQ( type, CASUAL_FIELD_SHORT);
         EXPECT_TRUE( CasualFieldTypeOfName( "long", &type) == CASUAL_FIELD_SUCCESS);
         EXPECT_EQ( type, CASUAL_FIELD_LONG);
         EXPECT_TRUE( CasualFieldTypeOfName( "char", &type) == CASUAL_FIELD_SUCCESS);
         EXPECT_EQ( type, CASUAL_FIELD_CHAR);
         EXPECT_TRUE( CasualFieldTypeOfName( "float", &type) == CASUAL_FIELD_SUCCESS);
         EXPECT_EQ( type, CASUAL_FIELD_FLOAT);
         EXPECT_TRUE( CasualFieldTypeOfName( "double", &type) == CASUAL_FIELD_SUCCESS);
         EXPECT_EQ( type, CASUAL_FIELD_DOUBLE);
         EXPECT_TRUE( CasualFieldTypeOfName( "string", &type) == CASUAL_FIELD_SUCCESS);
         EXPECT_EQ( type, CASUAL_FIELD_STRING);
         EXPECT_TRUE( CasualFieldTypeOfName( "binary", &type) == CASUAL_FIELD_SUCCESS);
         EXPECT_EQ( type, CASUAL_FIELD_BINARY);
      }

      TEST_F( casual_field_buffer_repository, get_type_of_invalid_name__expecting_invalid_type)
      {
         int type = -1;
         EXPECT_TRUE( CasualFieldTypeOfName( "666", &type) == CASUAL_FIELD_INVALID_TYPE);
      }

      TEST_F( casual_field_buffer_repository, get_type_of_name_to_null__expecting_success_and_no_crasch)
      {
         EXPECT_TRUE( CasualFieldTypeOfName( "string", nullptr) == CASUAL_FIELD_SUCCESS);
      }

      TEST( casual_field_buffer, add_three_remove_two_field_and_then_iterate__expecting_third_as_first)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 512);
         ASSERT_TRUE( buffer != nullptr);

         EXPECT_TRUE( CasualFieldAddShort( buffer, FLD_SHORT1, 123) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( CasualFieldAddLong( buffer, FLD_LONG1, 123456) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( CasualFieldAddFloat( buffer, FLD_FLOAT1, 3.14) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( CasualFieldRemoveOccurrence( buffer, FLD_SHORT1, 0) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( CasualFieldRemoveOccurrence( buffer, FLD_LONG1, 0) == CASUAL_FIELD_SUCCESS);

         long id = CASUAL_FIELD_NO_ID;
         long occurrence;

         EXPECT_TRUE( CasualFieldNext( buffer, &id, &occurrence) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( id == FLD_FLOAT1);
         EXPECT_TRUE( occurrence == 0);

         EXPECT_TRUE( CasualFieldNext( buffer, &id, &occurrence) == CASUAL_FIELD_NO_OCCURRENCE);

         tpfree( buffer);
      }


      TEST( casual_field_buffer, add_three_remove_middle_field_and_then_iterate__expecting_third_as_second)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 512);
         ASSERT_TRUE( buffer != nullptr);

         EXPECT_TRUE( CasualFieldAddShort( buffer, FLD_SHORT1, 123) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( CasualFieldAddLong( buffer, FLD_LONG1, 123456) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( CasualFieldAddFloat( buffer, FLD_FLOAT1, 3.14) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( CasualFieldRemoveOccurrence( buffer, FLD_LONG1, 0) == CASUAL_FIELD_SUCCESS);

         long id = CASUAL_FIELD_NO_ID;
         long occurrence;

         EXPECT_TRUE( CasualFieldNext( buffer, &id, &occurrence) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( id == FLD_SHORT1);
         EXPECT_TRUE( occurrence == 0);

         EXPECT_TRUE( CasualFieldNext( buffer, &id, &occurrence) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( id == FLD_FLOAT1);
         EXPECT_TRUE( occurrence == 0);

         EXPECT_TRUE( CasualFieldNext( buffer, &id, &occurrence) == CASUAL_FIELD_NO_OCCURRENCE);

         tpfree( buffer);
      }

      TEST( casual_field_buffer, get_pod_size__expecting_correct_sizes)
      {

         long size = 0;
         EXPECT_FALSE( CasualFieldPlainTypeHostSize( CASUAL_FIELD_SHORT, &size));
         EXPECT_TRUE( size == sizeof( short));
         EXPECT_FALSE( CasualFieldPlainTypeHostSize( CASUAL_FIELD_LONG, &size));
         EXPECT_TRUE( size == sizeof( long));
         EXPECT_FALSE( CasualFieldPlainTypeHostSize( CASUAL_FIELD_CHAR, &size));
         EXPECT_TRUE( size == sizeof( char));
         EXPECT_FALSE( CasualFieldPlainTypeHostSize( CASUAL_FIELD_FLOAT, &size));
         EXPECT_TRUE( size == sizeof( float));
         EXPECT_FALSE( CasualFieldPlainTypeHostSize( CASUAL_FIELD_DOUBLE, &size));
         EXPECT_TRUE( size == sizeof( double));
         EXPECT_TRUE( CasualFieldPlainTypeHostSize( CASUAL_FIELD_STRING, &size) == CASUAL_FIELD_INVALID_TYPE);
         EXPECT_TRUE( CasualFieldPlainTypeHostSize( CASUAL_FIELD_BINARY, &size) == CASUAL_FIELD_INVALID_TYPE);
         EXPECT_TRUE( CasualFieldPlainTypeHostSize( CASUAL_FIELD_SHORT, nullptr) == CASUAL_FIELD_INVALID_ARGUMENT);

      }


      TEST( casual_field_buffer, add_values_as_void_and_get_as_type__expecting_success)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 512);
         ASSERT_TRUE( buffer != nullptr);

         {
            short source = 123;
            EXPECT_FALSE( CasualFieldAddValue( buffer, FLD_SHORT1, &source, 0));
            short target{};
            EXPECT_FALSE( CasualFieldGetShort( buffer, FLD_SHORT1, 0, &target));
            EXPECT_TRUE( source == target);
         }

         {
            long source = 123456;
            EXPECT_FALSE( CasualFieldAddValue( buffer, FLD_LONG1, &source, 0));
            long target{};
            EXPECT_FALSE( CasualFieldGetLong( buffer, FLD_LONG1, 0, &target));
            EXPECT_TRUE( source == target);
         }

         {
            char source = 'a';
            EXPECT_FALSE( CasualFieldAddValue( buffer, FLD_CHAR1, &source, 0));
            char target{};
            EXPECT_FALSE( CasualFieldGetChar( buffer, FLD_CHAR1, 0, &target));
            EXPECT_TRUE( source == target);
         }

         {
            float source = 3.14;
            EXPECT_FALSE( CasualFieldAddValue( buffer, FLD_FLOAT1, &source, 0));
            float target{};
            EXPECT_FALSE( CasualFieldGetFloat( buffer, FLD_FLOAT1, 0, &target));
            EXPECT_TRUE( source == target);
         }

         {
            double source = 123.456;
            EXPECT_FALSE( CasualFieldAddValue( buffer, FLD_DOUBLE1, &source, 0));
            double target{};
            EXPECT_FALSE( CasualFieldGetDouble( buffer, FLD_DOUBLE1, 0, &target));
            EXPECT_TRUE( source == target);
         }

         {
            const char* source = "Hello World!";
            EXPECT_FALSE( CasualFieldAddValue( buffer, FLD_STRING1, source, 0));
            const char* target{};
            EXPECT_FALSE( CasualFieldGetString( buffer, FLD_STRING1, 0, &target));
            EXPECT_STREQ( source, target);
         }

         {
            const char source[] = "Hello World!";
            EXPECT_FALSE( CasualFieldAddValue( buffer, FLD_BINARY1, source, sizeof(source)));
            const char* target{};
            long count{};
            EXPECT_FALSE( CasualFieldGetBinary( buffer, FLD_BINARY1, 0, &target, &count));
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
            EXPECT_FALSE( CasualFieldAddShort( buffer, FLD_SHORT1, source));
            short target{};
            EXPECT_FALSE( CasualFieldGetValue( buffer, FLD_SHORT1, 0, &target, nullptr));
            EXPECT_TRUE( source == target);
         }

         {
            long source = 123456;
            EXPECT_FALSE( CasualFieldAddLong( buffer, FLD_LONG1, source));
            long target{};
            EXPECT_FALSE( CasualFieldGetValue( buffer, FLD_LONG1, 0, &target, nullptr));
            EXPECT_TRUE( source == target);
         }

         {
            char source = 'a';
            EXPECT_FALSE( CasualFieldAddChar( buffer, FLD_CHAR1, source));
            char target{};
            EXPECT_FALSE( CasualFieldGetValue( buffer, FLD_CHAR1, 0, &target, nullptr));
            EXPECT_TRUE( source == target);
         }

         {
            float source = 3.14;
            EXPECT_FALSE( CasualFieldAddFloat( buffer, FLD_FLOAT1, source));
            float target{};
            EXPECT_FALSE( CasualFieldGetValue( buffer, FLD_FLOAT1, 0, &target, nullptr));
            EXPECT_TRUE( source == target);
         }

         {
            double source = 123.456;
            EXPECT_FALSE( CasualFieldAddDouble( buffer, FLD_DOUBLE1, source));
            double target{};
            EXPECT_FALSE( CasualFieldGetValue( buffer, FLD_DOUBLE1, 0, &target, nullptr));
            EXPECT_TRUE( source == target);
         }

         {
            const char source[] = "Hello World!";
            EXPECT_FALSE( CasualFieldAddString( buffer, FLD_STRING1, source));
            char target[64];
            long count = sizeof( target);
            EXPECT_FALSE( CasualFieldGetValue( buffer, FLD_STRING1, 0, target, &count));
            EXPECT_STREQ( source, target);
            EXPECT_TRUE( count == sizeof( source));
         }

         {
            const char source[] = "Hello World!";
            EXPECT_FALSE( CasualFieldAddBinary( buffer, FLD_BINARY1, source, sizeof( source)));
            char target[64];
            long count = sizeof( target);
            EXPECT_FALSE( CasualFieldGetValue( buffer, FLD_STRING1, 0, target, &count));
            EXPECT_STREQ( source, target);
            EXPECT_TRUE( count == sizeof( source));
         }

         {
            const char source[] = "Hello World!";
            EXPECT_FALSE( CasualFieldAddString( buffer, FLD_STRING1, source));
            char target[4];
            long count = sizeof( target);
            EXPECT_TRUE( CasualFieldGetValue( buffer, FLD_STRING1, 0, target, &count) == CASUAL_FIELD_NO_SPACE);
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
               EXPECT_FALSE( CasualFieldAddShort( buffer, FLD_SHORT1, source));
               short target{};
               EXPECT_FALSE( CasualFieldGetShort( buffer, FLD_SHORT1, 0, &target));
               EXPECT_TRUE( source == target);
            }

            {
               short source = 321;
               EXPECT_FALSE( CasualFieldUpdateShort( buffer, FLD_SHORT1, 0, source));
               short target{};
               EXPECT_FALSE( CasualFieldGetShort( buffer, FLD_SHORT1, 0, &target));
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
               EXPECT_FALSE( CasualFieldAddString( buffer, FLD_STRING1, source));
               const char* target{};
               EXPECT_FALSE( CasualFieldGetString( buffer, FLD_STRING1, 0, &target));
               EXPECT_STREQ( source, target);
            }

            const char other[] = "An other string";
            EXPECT_FALSE( CasualFieldAddString( buffer, FLD_STRING1, other));

            {
               const char* target{};
               EXPECT_FALSE( CasualFieldGetString( buffer, FLD_STRING1, 1, &target));
               EXPECT_STREQ( other, target);
            }

            {
               const char source[] = "A looooooooooooooong string";
               EXPECT_FALSE( CasualFieldUpdateString( buffer, FLD_STRING1, 0, source));
               const char* target{};
               EXPECT_FALSE( CasualFieldGetString( buffer, FLD_STRING1, 0, &target));
               EXPECT_STREQ( source, target);
            }


            {
               const char* target{};
               EXPECT_FALSE( CasualFieldGetString( buffer, FLD_STRING1, 1, &target));
               EXPECT_STREQ( other, target);
            }


            {
               const char source[] = "A tiny string";
               EXPECT_FALSE( CasualFieldUpdateString( buffer, FLD_STRING1, 0, source));
               const char* target{};
               EXPECT_FALSE( CasualFieldGetString( buffer, FLD_STRING1, 0, &target));
               EXPECT_STREQ( source, target);
            }

            {
               const char* target{};
               EXPECT_FALSE( CasualFieldGetString( buffer, FLD_STRING1, 1, &target));
               EXPECT_STREQ( other, target);
            }


         }

         tpfree( buffer);
      }


      TEST( casual_field_buffer, count_occurrences__expecting_correct_numbers)
      {
         auto buffer = tpalloc( CASUAL_FIELD, "", 512);
         ASSERT_TRUE( buffer != nullptr);

         EXPECT_FALSE( CasualFieldAddString( buffer, FLD_STRING1, "First string 1"));
         EXPECT_FALSE( CasualFieldAddString( buffer, FLD_STRING2, "First string 2"));
         EXPECT_FALSE( CasualFieldAddString( buffer, FLD_STRING1, "Other string 1"));

         {
            long occurrences{};
            EXPECT_FALSE( CasualFieldOccurrencesOfId( buffer, FLD_STRING1, &occurrences));
            EXPECT_TRUE( occurrences == 2);
         }

         {
            long occurrences{};
            EXPECT_FALSE( CasualFieldOccurrencesInBuffer( buffer, &occurrences));
            EXPECT_TRUE( occurrences == 3);
         }

         tpfree( buffer);
      }







   }

}

