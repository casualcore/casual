//!
//! casual_isolatedunittest_archive.cpp
//!
//! Created on: Oct 14, 2012
//!     Author: Lazan
//!


#include <gtest/gtest.h>


#include "sf/namevaluepair.h"
#include "sf/basic_archive.h"


#include <string>

#include <typeinfo>



namespace casual
{

   struct TestPolicyBase
   {
      TestPolicyBase() {}

      TestPolicyBase( const TestPolicyBase& rhs) : m_buffer( rhs.m_buffer) {}


      void handle_start( const char* name)
      {

      }

      void handle_end( const char* name)
      {

      }

      void handle_container_end()
      {

      }

      void handle_serialtype_start()
      {

      }

      void handle_serialtype_end()
      {

      }


      std::vector< char> m_buffer = std::vector< char>( 64);
      std::size_t m_offset = 0;
   };


   struct TestReaderPolicy : public TestPolicyBase
   {
      template< typename T>
      TestReaderPolicy( T&& value) : TestPolicyBase( std::forward< T>( value)) {}

      template< typename T>
      void read( T& value, std::size_t size)
      {

         char* data = reinterpret_cast< char*>( &value);

         auto start = m_buffer.begin() + m_offset;

         std::copy( start, start + size, data);

         m_offset += size;
      }

      template< typename T>
      void read( T& value)
      {
         read( value, sizeof( T));
      }

      void read( std::string& value)
      {
         std::size_t size;
         read( size);

         auto start = m_buffer.begin() + m_offset;

         value.assign( start, start + size);

         m_offset += size;
      }

      std::size_t handle_container_start( std::size_t size)
      {
         read( size);
         return size;
      }


      void read( std::wstring& value)
      {
         // do nadas
      }

      void read( std::vector< char>& value)
      {
         // do nada
      }
   };


   struct TestWriterPolicy : public TestPolicyBase
   {
      TestWriterPolicy() {}

      template< typename T>
      TestWriterPolicy( T&& value) : TestPolicyBase( std::forward< T>( value)) {}
      //using TestPolicyBase::TestPolicyBase;

      void write( const char* data, std::size_t size)
      {

         while( m_offset + size > m_buffer.size() )
         {
            m_buffer.resize( m_buffer.size() * 2);
         }

         std::copy( data, data + size, ( &m_buffer[ 0]) + m_offset);

         m_offset += size;
      }



      template< typename T>
      void write( const T& value)
      {
         write( reinterpret_cast< const char*>( &value), sizeof( T));
      }

      void write( const std::string& value)
      {
         std::size_t size = value.size();
         write( size);

         write( value.data(), size);

      }

      std::size_t handle_container_start( std::size_t size)
      {
         write( size);
         return size;
      }


      void write( const std::vector< char>& value)
      {
         // do nada
      }
   };




   TEST( casual_sf_ArchiveWriter_serialize, pod)
   {

      sf::archive::basic_writer< TestWriterPolicy> writer;

      writer << CASUAL_MAKE_NVP( 10);
   }


   TEST( casual_sf_archive_serialize, pod)
   {

      sf::archive::basic_writer< TestWriterPolicy> writer;

      writer << CASUAL_MAKE_NVP( 34L);

      sf::archive::basic_reader< TestReaderPolicy> reader( writer.implementation());

      long result;

      reader >> CASUAL_MAKE_NVP( result);

      EXPECT_TRUE( result == 34) << "result: " << result;

   }



   TEST( casual_sf_ArchiveWriter_serialize, vector_long)
   {

      sf::archive::basic_writer< TestWriterPolicy> writer;

      std::vector< long> someInts = { 1, 2, 3, 4 };

      writer << CASUAL_MAKE_NVP( someInts);

      std::vector< long> result;

      sf::archive::basic_reader< TestReaderPolicy> reader( writer.implementation());

      reader >> CASUAL_MAKE_NVP( result);

      ASSERT_TRUE( result.size() == 4) << "result.size(): " << result.size();
      EXPECT_TRUE( result.at( 0) == 1);
      EXPECT_TRUE( result.at( 1) == 2);
      EXPECT_TRUE( result.at( 2) == 3);
      EXPECT_TRUE( result.at( 3) == 4);

   }


   TEST( casual_sf_ArchiveWriter_serialize, map_long_string)
   {

      sf::archive::basic_writer< TestWriterPolicy> writer;

      std::map< long, std::string> value = { { 1, "test 1"}, { 2, "test 2"}, { 3, "test 3"}, { 4, "test 4"} };

      writer << CASUAL_MAKE_NVP( value);


      std::map< long, std::string> result;

      sf::archive::basic_reader< TestReaderPolicy> reader( writer.implementation());

      reader >> CASUAL_MAKE_NVP( result);

      ASSERT_TRUE( result.size() == 4) << "result.size(): " << result.size();
      EXPECT_TRUE( result.at( 1) == "test 1");
      EXPECT_TRUE( result.at( 2) == "test 2");
      EXPECT_TRUE( result.at( 3) == "test 3");
      EXPECT_TRUE( result.at( 4) == "test 4");


   }

   struct Serializible
   {


      std::string someString;
      long someLong;

      template< typename A>
      void serialize( A& archive)
      {
         archive << CASUAL_MAKE_NVP( someString);
         archive << CASUAL_MAKE_NVP( someLong);
      }
   };

   TEST( casual_sf_ArchiveWriter_serialize, serializible)
   {

      sf::archive::basic_writer< TestWriterPolicy> writer;

      Serializible value;
      value.someLong = 23;
      value.someString = "kdjlfskjf";

      writer << CASUAL_MAKE_NVP( value);
   }


}

