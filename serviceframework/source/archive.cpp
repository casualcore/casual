

#include "sf/archive.h"

namespace casual
{
   namespace sf
   {
      namespace archive
      {


         Base::Base()
         {

         }

         Base::~Base()
         {

         }

         void Base::handleStart( const char* name)
         {
            handle_start( name);
         }

         void Base::handleEnd( const char* name)
         {
            handle_end( name);
         }

         std::size_t Base::handleContainerStart( std::size_t size)
         {
            return handle_container_start( size);
         }

         void Base::handleContainerEnd()
         {
            handle_container_end();
         }

         void Base::handleSerialtypeStart()
         {
            handle_serialtype_start();
         }

         void Base::handleSerialtypeEnd()
         {
            handle_serialtype_end();
         }



         Reader::Reader()
         {

         }

         Reader::~Reader()
         {

         }

         void Reader::read( bool& value)
         {
            readPOD( value);
         }

         void Reader::read( char& value)
         {
            readPOD( value);
         }

         void Reader::read( short& value)
         {
            readPOD( value);
         }

         void Reader::read( int& value)
         {
            long temp;
            readPOD( temp);
            value = temp;
         }

         //void read (const unsigned int& value);

         void Reader::read( long& value)
         {
            readPOD( value);
         }

         void Reader::read (unsigned long& value)
         {
            long temp;
            readPOD( temp);
            value = temp;
         }

         void Reader::read( float& value)
         {
            readPOD( value);
         }

         void Reader::read( double& value)
         {
            readPOD( value);
         }

         void Reader::read( std::string& value)
         {
            readPOD( value);
         }

         void Reader::read( common::binary_type& value)
         {
            readPOD( value);
         }



         Writer::Writer()
         {

         }

         Writer::~Writer()
         {

         }

         void Writer::write( const bool value)
         {
            writePOD( value);
         }

         void Writer::write( const char value)
         {
            writePOD( value);
         }

         void Writer::write( const short value)
         {
            writePOD( value);
         }

         void Writer::write( const int value)
         {
            writePOD( static_cast< long>( value));
         }

         //void write (const unsigned int& value);

         void Writer::write( const long value)
         {
            writePOD( value);
         }

         void Writer::write (const unsigned long value)
         {
            writePOD( static_cast< long>( value));
         }

         void Writer::write( const float value)
         {
            writePOD( value);
         }

         void Writer::write( const double value)
         {
            writePOD( value);
         }

         void Writer::write( const std::string& value)
         {
            writePOD( value);
         }

         void Writer::write( const common::binary_type& value)
         {
            writePOD( value);
         }


      }
   }
}

