//!
//! casual_isolatedunittest_archive.cpp
//!
//! Created on: Oct 14, 2012
//!     Author: Lazan
//!


#include <gtest/gtest.h>


#include "casual_namevaluepair.h"
#include "casual_basic_writer.h"

#include <typeinfo>



namespace casual
{

   struct TestWriteArchivePolicy
   {
      TestWriteArchivePolicy( std::ostream& out) : m_ostream( out) {}

      void handle_start( const char* name)
      {
         m_ostream << "handle_start( " << name << ")" << std::endl;
      }

      void handle_end( const char* name)
      {
         m_ostream << "handle_end( " << name << ")" << std::endl;
      }

      void handle_container_start()
      {
         m_ostream << "handle_container_start()" << std::endl;
      }

      void handle_container_end()
      {
         m_ostream << "handle_container_end()" << std::endl;
      }

      void handle_serialtype_start()
      {
         m_ostream << "handle_serialtype_start()"<< std::endl;
      }

      void handle_serialtype_end()
      {
         m_ostream << "handle_serialtype_end()" << std::endl;
      }


      template< typename T>
      void write( T&& value)
      {
         m_ostream << "type: " << typeid( T).name() << " - value;" << value << std::endl;
      }

      void write( const std::wstring& value)
      {
         // do nadas
      }

      void write( const std::vector< char>& value)
      {
         // do nada
      }


      std::ostream& m_ostream;
   };



   TEST( casual_sf_ArchiveWriter_serialize, pod)
   {

      sf::archive::basic_writer< TestWriteArchivePolicy> writer( std::cout);


      writer << CASUAL_MAKE_NVP( 10);
   }

   TEST( casual_sf_ArchiveWriter_serialize, pod_container)
   {

      sf::archive::basic_writer< TestWriteArchivePolicy> writer( std::cout);

      std::vector< int> someInts = { 1, 2, 3, 4 };

      writer << CASUAL_MAKE_NVP( someInts);
   }

   struct Serializible
   {


      std::string someString;
      long someLong;

      template< typename A>
      void serialize( A& archive) const
      {
         archive << CASUAL_MAKE_NVP( someString);
         archive << CASUAL_MAKE_NVP( someLong);
      }
   };

   TEST( casual_sf_ArchiveWriter_serialize, serializible)
   {

      sf::archive::basic_writer< TestWriteArchivePolicy> writer( std::cout);

      Serializible value;
      value.someLong = 23;
      value.someString = "kdjlfskjf";

      writer << CASUAL_MAKE_NVP( value);
   }


}

