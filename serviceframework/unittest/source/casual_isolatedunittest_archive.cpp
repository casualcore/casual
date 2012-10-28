//!
//! casual_isolatedunittest_archive.cpp
//!
//! Created on: Oct 14, 2012
//!     Author: Lazan
//!


#include <gtest/gtest.h>


#include "casual_namevaluepair.h"
#include "casual_basic_writer.h"
#include "casual_basic_reader.h"

#include <typeinfo>



namespace casual
{

   struct TestPolicy
   {
      //TestPolicy( ) : m_ostream( out) {}


      void handle_start( const char* name)
      {

      }

      void handle_end( const char* name)
      {

      }

      void handle_container_start()
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


      template< typename T>
      void write( T&& value)
      {
         m_stream << value;
      }

      void write( const std::wstring& value)
      {
         // do nadas
      }

      void write( const std::vector< char>& value)
      {
         // do nada
      }


      template< typename T>
      void read( T& value)
      {
         m_stream >> value;
      }

      void read( std::wstring& value)
      {
         // do nadas
      }

      void read( std::vector< char>& value)
      {
         // do nada
      }


      std::stringstream m_stream;
   };



   TEST( casual_sf_ArchiveWriter_serialize, pod)
   {

      sf::archive::basic_writer< TestPolicy> writer;

      writer << CASUAL_MAKE_NVP( 10);
   }



   TEST( casual_sf_ArchiveWriter_serialize, pod_container)
   {

      sf::archive::basic_writer< TestPolicy> writer;

      std::vector< int> someInts = { 1, 2, 3, 4 };

      writer << CASUAL_MAKE_NVP( someInts);

      std::vector< int> result;

      sf::archive::basic_reader< TestPolicy> reader( writer.policy());

      reader >> CASUAL_MAKE_NVP( result);

      ASSERT_TRUE( result.size() == 4) << "result.size(): " << result.size();

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

      sf::archive::basic_writer< TestPolicy> writer;

      Serializible value;
      value.someLong = 23;
      value.someString = "kdjlfskjf";

      writer << CASUAL_MAKE_NVP( value);
   }


}

