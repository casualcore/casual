//
// test_field_buffer.cpp
//
//  Created on: 12 nov 2013
//      Author: Kristone
//


#include <gtest/gtest.h>

#include "common/field_buffer.h"


#include <string>



namespace casual
{

   namespace common
   {
      namespace
      {

         const auto FLD_SHORT1 = CASUAL_FIELD_SHORT * CASUAL_FIELD_TYPE_BASE + 1;
         const auto FLD_SHORT2 = CASUAL_FIELD_SHORT * CASUAL_FIELD_TYPE_BASE + 2;
         const auto FLD_SHORT3 = CASUAL_FIELD_SHORT * CASUAL_FIELD_TYPE_BASE + 3;

         const auto FLD_LONG1 = CASUAL_FIELD_LONG * CASUAL_FIELD_TYPE_BASE + 1;
         const auto FLD_LONG2 = CASUAL_FIELD_LONG * CASUAL_FIELD_TYPE_BASE + 2;
         const auto FLD_LONG3 = CASUAL_FIELD_LONG * CASUAL_FIELD_TYPE_BASE + 3;

         const auto FLD_CHAR1 = CASUAL_FIELD_CHAR * CASUAL_FIELD_TYPE_BASE + 1;
         const auto FLD_CHAR2 = CASUAL_FIELD_CHAR * CASUAL_FIELD_TYPE_BASE + 2;
         const auto FLD_CHAR3 = CASUAL_FIELD_CHAR * CASUAL_FIELD_TYPE_BASE + 3;

         const auto FLD_FLOAT1 = CASUAL_FIELD_FLOAT * CASUAL_FIELD_TYPE_BASE + 1;
         const auto FLD_FLOAT2 = CASUAL_FIELD_FLOAT * CASUAL_FIELD_TYPE_BASE + 2;
         const auto FLD_FLOAT3 = CASUAL_FIELD_FLOAT * CASUAL_FIELD_TYPE_BASE + 3;

         const auto FLD_DOUBLE1 = CASUAL_FIELD_DOUBLE * CASUAL_FIELD_TYPE_BASE + 1;
         const auto FLD_DOUBLE2 = CASUAL_FIELD_DOUBLE * CASUAL_FIELD_TYPE_BASE + 2;
         const auto FLD_DOUBLE3 = CASUAL_FIELD_DOUBLE * CASUAL_FIELD_TYPE_BASE + 3;

         const auto FLD_STRING1 = CASUAL_FIELD_STRING * CASUAL_FIELD_TYPE_BASE + 1;
         const auto FLD_STRING2 = CASUAL_FIELD_STRING * CASUAL_FIELD_TYPE_BASE + 2;
         const auto FLD_STRING3 = CASUAL_FIELD_STRING * CASUAL_FIELD_TYPE_BASE + 3;

         const auto FLD_BINARY1 = CASUAL_FIELD_BINARY * CASUAL_FIELD_TYPE_BASE + 1;
         const auto FLD_BINARY2 = CASUAL_FIELD_BINARY * CASUAL_FIELD_TYPE_BASE + 2;
         const auto FLD_BINARY3 = CASUAL_FIELD_BINARY * CASUAL_FIELD_TYPE_BASE + 3;
      }


      TEST( casual_field_buffer, allocate_with_enough_size__expecting_success)
      {
         char buffer[512];
         EXPECT_TRUE( CasualFieldCreate( buffer, sizeof(buffer)) == CASUAL_FIELD_SUCCESS);
      }

      TEST( casual_field_buffer, reallocate_with_bigger_size__expecting_success)
      {
         char buffer[64];
         ASSERT_TRUE( CasualFieldCreate( buffer, sizeof(buffer) / 2) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( CasualFieldExpand( buffer, sizeof(buffer)) == CASUAL_FIELD_SUCCESS);
      }

      TEST( casual_field_buffer, reallocate_with_smaller_size__expecting_success)
      {
         char buffer[64];
         ASSERT_TRUE( CasualFieldCreate( buffer, sizeof(buffer)) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( CasualFieldReduce( buffer, sizeof(buffer) / 2) == CASUAL_FIELD_SUCCESS);
      }

      TEST( casual_field_buffer, reallocate_with_smaller_size__expecting_failure)
      {
         char buffer[64];
         ASSERT_TRUE( CasualFieldCreate( buffer, sizeof(buffer)) == CASUAL_FIELD_SUCCESS);
         ASSERT_TRUE( CasualFieldAddString( buffer, FLD_STRING1, "goin' casual today!") == CASUAL_FIELD_SUCCESS);

         long used = 0;
         ASSERT_TRUE( CasualFieldExploreBuffer( buffer, 0, &used) == CASUAL_FIELD_SUCCESS);

         EXPECT_TRUE( CasualFieldReduce( buffer, used - 1) == CASUAL_FIELD_NO_SPACE);
      }


      TEST( casual_field_buffer, add_and_get__expecting_success)
      {
         char buffer[512];
         ASSERT_TRUE( CasualFieldCreate( buffer, sizeof(buffer)) == CASUAL_FIELD_SUCCESS);

         EXPECT_TRUE( CasualFieldAddChar( buffer, FLD_CHAR1, 'a') == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( CasualFieldAddShort( buffer, FLD_SHORT1, 123) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( CasualFieldAddLong( buffer, FLD_LONG1, 654321) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( CasualFieldAddFloat( buffer, FLD_FLOAT1, 3.14) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( CasualFieldAddDouble( buffer, FLD_DOUBLE1, 987.654) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( CasualFieldAddString( buffer, FLD_STRING1, "Hello!") == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( CasualFieldAddBinary( buffer, FLD_BINARY1, "Some Data", 9) == CASUAL_FIELD_SUCCESS);

         char character;
         EXPECT_TRUE( CasualFieldGetChar( buffer, FLD_CHAR1, 0, &character) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( character == 'a');

         short short_integer;
         EXPECT_TRUE( CasualFieldGetShort( buffer, FLD_SHORT1, 0, &short_integer) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( short_integer == 123);

         long long_integer;
         EXPECT_TRUE( CasualFieldGetLong( buffer, FLD_LONG1, 0, &long_integer) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( long_integer == 654321);

         float short_decimal;
         EXPECT_TRUE( CasualFieldGetFloat( buffer, FLD_FLOAT1, 0, &short_decimal) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( short_decimal > 3.1 && short_decimal < 3.2);

         double long_decimal;
         EXPECT_TRUE( CasualFieldGetDouble( buffer, FLD_DOUBLE1, 0, &long_decimal) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( long_decimal > 987.6 && long_decimal < 987.7);

         const char* string;
         EXPECT_TRUE( CasualFieldGetString( buffer, FLD_STRING1, 0, &string) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( std::string( string) == "Hello!") << std::string( string);

         const char* binary;
         long size;
         EXPECT_TRUE( CasualFieldGetBinary( buffer, FLD_BINARY1, 0, &binary, &size) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( std::string( binary, size) == "Some Data") << std::string( binary, size);
         EXPECT_TRUE( size == 9);

      }


      TEST( casual_field_buffer, add_and_get_several_occurrences__expecting_success)
      {
         char buffer[512];
         ASSERT_TRUE( CasualFieldCreate( buffer, sizeof(buffer)) == CASUAL_FIELD_SUCCESS);

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
         EXPECT_TRUE( std::string( string) == "Hello!") << std::string( string);

         EXPECT_TRUE( CasualFieldGetString( buffer, FLD_STRING1, 1, &string) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( std::string( string) == "!olleH") << std::string( string);

      }


      TEST( casual_field_buffer, add_one_and_get_two_occurrences__expecting_no_occurrence)
      {
         char buffer[512];
         ASSERT_TRUE( CasualFieldCreate( buffer, sizeof(buffer)) == CASUAL_FIELD_SUCCESS);

         EXPECT_TRUE( CasualFieldAddShort( buffer, FLD_SHORT1, 123) == CASUAL_FIELD_SUCCESS);

         short short_integer;
         EXPECT_TRUE( CasualFieldGetShort( buffer, FLD_SHORT1, 0, &short_integer) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( CasualFieldGetShort( buffer, FLD_SHORT1, 1, &short_integer) == CASUAL_FIELD_NO_OCCURRENCE);
      }

      TEST( casual_field_buffer, add_short_with_id_of_long__expecting_invalid_id)
      {
         char buffer[512];
         ASSERT_TRUE( CasualFieldCreate( buffer, sizeof(buffer)) == CASUAL_FIELD_SUCCESS);

         EXPECT_TRUE( CasualFieldAddShort( buffer, FLD_LONG1, 123) == CASUAL_FIELD_INVALID_ID);
      }

      TEST( casual_field_buffer, add_short_with_invalid_id__expecting_invalid_id)
      {
         char buffer[512];
         ASSERT_TRUE( CasualFieldCreate( buffer, sizeof(buffer)) == CASUAL_FIELD_SUCCESS);

         EXPECT_TRUE( CasualFieldAddShort( buffer, -1, 123) == CASUAL_FIELD_INVALID_ID);
         EXPECT_TRUE( CasualFieldAddShort( buffer, 0, 123) == CASUAL_FIELD_INVALID_ID);
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
         char buffer[512];
         ASSERT_TRUE( CasualFieldCreate( buffer, sizeof(buffer)) == CASUAL_FIELD_SUCCESS);

         EXPECT_TRUE( CasualFieldRemoveOccurrence( buffer, 0, 0) == CASUAL_FIELD_INVALID_ID);
         EXPECT_TRUE( CasualFieldRemoveOccurrence( buffer, -1, 0) == CASUAL_FIELD_INVALID_ID);
      }


      TEST( casual_field_buffer, remove_occurrence_with_no_occurrence__expecting_no_occurrence)
      {
         char buffer[512];
         ASSERT_TRUE( CasualFieldCreate( buffer, sizeof(buffer)) == CASUAL_FIELD_SUCCESS);

         EXPECT_TRUE( CasualFieldExist( buffer, FLD_LONG1, 0) == CASUAL_FIELD_NO_OCCURRENCE);
         EXPECT_TRUE( CasualFieldRemoveOccurrence( buffer, FLD_LONG1, 0) == CASUAL_FIELD_NO_OCCURRENCE);
      }

      TEST( casual_field_buffer, add_and_remove_occurrence__expecting_success)
      {
         char buffer[512];
         ASSERT_TRUE( CasualFieldCreate( buffer, sizeof(buffer)) == CASUAL_FIELD_SUCCESS);

         EXPECT_TRUE( CasualFieldAddShort( buffer, FLD_SHORT1, 123) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( CasualFieldRemoveOccurrence( buffer, FLD_SHORT1, 0) == CASUAL_FIELD_SUCCESS);
      }

      TEST( casual_field_buffer, add_twp_and_remove_first_occurrence_and_get_second_as_first__expecting_success)
      {
         char buffer[512];
         ASSERT_TRUE( CasualFieldCreate( buffer, sizeof(buffer)) == CASUAL_FIELD_SUCCESS);

         EXPECT_TRUE( CasualFieldAddShort( buffer, FLD_SHORT1, 123) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( CasualFieldAddShort( buffer, FLD_SHORT1, 321) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( CasualFieldRemoveOccurrence( buffer, FLD_SHORT1, 0) == CASUAL_FIELD_SUCCESS);
         short short_integer;
         EXPECT_TRUE( CasualFieldGetShort( buffer, FLD_SHORT1, 0, &short_integer) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( short_integer == 321);
      }

      TEST( casual_field_buffer, add_twp_and_remove_second_occurrence_and_first_second_as_first__expecting_success)
      {
         char buffer[512];
         ASSERT_TRUE( CasualFieldCreate( buffer, sizeof(buffer)) == CASUAL_FIELD_SUCCESS);

         EXPECT_TRUE( CasualFieldAddShort( buffer, FLD_SHORT1, 123) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( CasualFieldAddShort( buffer, FLD_SHORT1, 321) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( CasualFieldRemoveOccurrence( buffer, FLD_SHORT1, 1) == CASUAL_FIELD_SUCCESS);
         short short_integer;
         EXPECT_TRUE( CasualFieldGetShort( buffer, FLD_SHORT1, 0, &short_integer) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( short_integer == 123);
      }


      TEST( casual_field_buffer, remove_all_occurrences_and_check_existence__expecting_no_occurrence)
      {
         char buffer[512];
         ASSERT_TRUE( CasualFieldCreate( buffer, sizeof(buffer)) == CASUAL_FIELD_SUCCESS);

         short short_integer;

         EXPECT_TRUE( CasualFieldAddShort( buffer, FLD_SHORT1, 123) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( CasualFieldAddShort( buffer, FLD_SHORT1, 321) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( CasualFieldExist( buffer, FLD_SHORT1, 0) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( CasualFieldRemoveId( buffer, FLD_SHORT1) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( CasualFieldExist( buffer, FLD_SHORT1, 0) == CASUAL_FIELD_NO_OCCURRENCE);
      }

      TEST( casual_field_buffer, add_to_source_and_copy_buffer_and_get_from_target__expecting_success)
      {
         char source[64];
         ASSERT_TRUE( CasualFieldCreate( source, sizeof(source)) == CASUAL_FIELD_SUCCESS);

         char target[128];
         ASSERT_TRUE( CasualFieldCreate( target, sizeof(target)) == CASUAL_FIELD_SUCCESS);

         short short_integer;

         ASSERT_TRUE( CasualFieldAddShort( source, FLD_SHORT1, 123) == CASUAL_FIELD_SUCCESS);
         ASSERT_TRUE( CasualFieldCopyBuffer( target, source) == CASUAL_FIELD_SUCCESS);
         ASSERT_TRUE( CasualFieldGetShort( target, FLD_SHORT1, 0, &short_integer) == CASUAL_FIELD_SUCCESS);

         EXPECT_TRUE( short_integer == 123);

      }


      TEST( casual_field_buffer, test_some_iteration)
      {
         char buffer[128];
         ASSERT_TRUE( CasualFieldCreate( buffer, sizeof(buffer)) == CASUAL_FIELD_SUCCESS);

         ASSERT_TRUE( CasualFieldAddShort( buffer, FLD_SHORT1, 123) == CASUAL_FIELD_SUCCESS);
         ASSERT_TRUE( CasualFieldAddLong( buffer, FLD_LONG1, 123456) == CASUAL_FIELD_SUCCESS);
         ASSERT_TRUE( CasualFieldAddLong( buffer, FLD_LONG1, 654321) == CASUAL_FIELD_SUCCESS);

         long id;
         long occurrence;


         EXPECT_TRUE( CasualFieldFirst( buffer, &id, &occurrence) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( id == FLD_SHORT1);
         EXPECT_TRUE( occurrence == 0);
         EXPECT_TRUE( CasualFieldNext( buffer, &id, &occurrence) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( id == FLD_LONG1);
         EXPECT_TRUE( occurrence == 0);
         EXPECT_TRUE( CasualFieldNext( buffer, &id, &occurrence) == CASUAL_FIELD_SUCCESS);
         EXPECT_TRUE( id == FLD_LONG1);
         EXPECT_TRUE( occurrence == 1);
         EXPECT_TRUE( CasualFieldNext( buffer, &id, &occurrence) == CASUAL_FIELD_NO_OCCURRENCE);


      }


   }

}

