//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include <common/unittest.h>


#include "common/serialize/macro.h"
#include "common/serialize/binary.h"
#include "common/serialize/native/binary.h"

#include "common/log.h"

#include "../../include/test_vo.h"


#include <string>

#include <typeinfo>



namespace casual
{

   TEST( serialize_binary_writer, archive_type)
   {
      common::unittest::Trace trace;

      auto writer = common::serialize::binary::writer();

      static_assert( common::serialize::archive::is::dynamic< decltype( writer)>);
      EXPECT_TRUE( writer.archive_properties() == common::serialize::archive::Property::order) << CASUAL_NAMED_VALUE( writer.archive_properties());
   }


   TEST( serialize_binary_writer, serialize_pod)
   {
      common::unittest::Trace trace;

      auto writer = common::serialize::native::binary::writer();
      writer << CASUAL_NAMED_VALUE( 10);
   }

   TEST( serialize_binary_writer, serialize_string)
   {
      common::unittest::Trace trace;

      auto writer = common::serialize::native::binary::writer();

      writer << CASUAL_NAMED_VALUE( std::string{ "test"});

      auto buffer = writer.consume();
      auto reader = common::serialize::native::binary::reader( buffer);

      std::string result;

      reader >> CASUAL_NAMED_VALUE( result);

      EXPECT_TRUE( result == "test") << "result: " << result;

   }


   TEST( serialize_binary_reader_writer, serialize_pod)
   {
      common::unittest::Trace trace;


      auto writer = common::serialize::native::binary::writer();
      writer << CASUAL_NAMED_VALUE( 34L);

      auto buffer = writer.consume();
      auto reader = common::serialize::native::binary::reader( buffer);

      long result;

      reader >> CASUAL_NAMED_VALUE( result);

      EXPECT_TRUE( result == 34) << "result: " << result;

   }



   TEST( serialize_binary_reader_writer, serialize_vector_long)
   {
      common::unittest::Trace trace;

      auto writer = common::serialize::native::binary::writer();

      std::vector< long> someInts = { 1, 2, 3, 4 };

      writer << CASUAL_NAMED_VALUE( someInts);

      std::vector< long> result;

      auto buffer = writer.consume();
      auto reader = common::serialize::native::binary::reader( buffer);

      reader >> CASUAL_NAMED_VALUE( result);

      ASSERT_TRUE( result.size() == 4) << "result.size(): " << result.size();
      EXPECT_TRUE( result.at( 0) == 1);
      EXPECT_TRUE( result.at( 1) == 2);
      EXPECT_TRUE( result.at( 2) == 3);
      EXPECT_TRUE( result.at( 3) == 4);

   }


   TEST( serialize_binary_reader_writer, map_long_string)
   {
      common::unittest::Trace trace;

      auto writer = common::serialize::native::binary::writer();

      std::map< long, std::string> value = { { 1, "test 1"}, { 2, "test 2"}, { 3, "test 3"}, { 4, "test 4"} };

      writer << CASUAL_NAMED_VALUE( value);


      std::map< long, std::string> result;

      auto buffer = writer.consume();
      auto reader = common::serialize::native::binary::reader( buffer);

      reader >> CASUAL_NAMED_VALUE( result);

      ASSERT_TRUE( result.size() == 4) << "result.size(): " << result.size();
      EXPECT_TRUE( result.at( 1) == "test 1") << "result.at( 1): " << result.at( 1);
      EXPECT_TRUE( result.at( 2) == "test 2");
      EXPECT_TRUE( result.at( 3) == "test 3");
      EXPECT_TRUE( result.at( 4) == "test 4");


   }

   struct Serializable
   {


      std::string someString;
      long someLong;

      CASUAL_CONST_CORRECT_SERIALIZE
      (
         CASUAL_SERIALIZE( someString);
         CASUAL_SERIALIZE( someLong);
      )
   };

   TEST( serialize_binary_reader_writer, serializable)
   {
      common::unittest::Trace trace;

      auto writer = common::serialize::native::binary::writer();

      {
         Serializable value;
         value.someLong = 23;
         value.someString = "kdjlfskjf";

         writer << CASUAL_NAMED_VALUE( value);
      }

      {
         auto buffer = writer.consume();
         auto reader = common::serialize::native::binary::reader( buffer);

         Serializable value;
         reader >> CASUAL_NAMED_VALUE( value);

         EXPECT_TRUE( value.someLong == 23);
         EXPECT_TRUE( value.someString == "kdjlfskjf");
      }

   }

   TEST( serialize_binary_reader_writer, complex_serializable)
   {
      common::unittest::Trace trace;

      auto writer = common::serialize::native::binary::writer();

      {
         test::Composite value;
         value.m_string = "test";
         value.m_values = { 1, 2, 3, 4};
         std::get< 0>( value.m_tuple) = 10;
         std::get< 1>( value.m_tuple) = "poop";
         std::get< 2>( value.m_tuple).m_short = 42;

         std::vector< test::Composite> range{ value, value, value, value};
         
         writer << CASUAL_NAMED_VALUE( range);
      }

      {
         auto buffer = writer.consume();
         auto reader = common::serialize::native::binary::reader( buffer);

         std::vector< test::Composite> range;
         reader >> CASUAL_NAMED_VALUE( range);

         ASSERT_TRUE( range.size() == 4);
         ASSERT_TRUE( range.at( 0).m_values.size() == 4);
         EXPECT_TRUE( range.at( 0).m_values.at( 0).m_long == 1);
         EXPECT_TRUE( range.at( 0).m_values.at( 1).m_long == 2);
         EXPECT_TRUE( range.at( 0).m_values.at( 2).m_long == 3);
         EXPECT_TRUE( range.at( 0).m_values.at( 3).m_long == 4);
         EXPECT_TRUE( std::get< 2>( range.at( 0).m_tuple).m_short == 42);
         EXPECT_TRUE( std::get< 2>( range.at( 3).m_tuple).m_short == 42);
      }

   }


}

