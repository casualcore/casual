

#include "sf/archive/archive.h"

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


         std::size_t Base::container_start( std::size_t size, const char* name)
         {
            return dispatch_container_start( size, name);
         }

         void Base::container_end( const char* name)
         {
            dispatch_container_end( name);
         }

         bool Base::serialtype_start( const char* name)
         {
            return dispatch_serialtype_start( name);
         }

         void Base::serialtype_end( const char* name)
         {
            dispatch_serialtype_end( name);
         }



         Reader::Reader()
         {

         }

         Reader::~Reader()
         {

         }

         void Reader::read( bool& value, const char* name)
         {
            pod( value, name);
         }

         void Reader::read( char& value, const char* name)
         {
            pod( value, name);
         }

         void Reader::read( short& value, const char* name)
         {
            pod( value, name);
         }

         void Reader::read( int& value, const char* name)
         {
            long temp;
            pod( temp, name);
            value = temp;
         }

         //void read (const unsigned int& value);

         void Reader::read( long& value, const char* name)
         {
            pod( value, name);
         }

         void Reader::read( long long& value, const char* name)
         {
            pod( value, name);
         }

         void Reader::read (unsigned long& value, const char* name)
         {
            long temp;
            pod( temp, name);
            value = temp;
         }

         void Reader::read( float& value, const char* name)
         {
            pod( value, name);
         }

         void Reader::read( double& value, const char* name)
         {
            pod( value, name);
         }

         void Reader::read( std::string& value, const char* name)
         {
            pod( value, name);
         }

         void Reader::read( platform::binary_type& value, const char* name)
         {
            pod( value, name);
         }



         Writer::Writer()
         {

         }

         Writer::~Writer()
         {

         }

         void Writer::write( const bool value, const char* name)
         {
            pod( value, name);
         }

         void Writer::write( const char value, const char* name)
         {
            pod( value, name);
         }

         void Writer::write( const short value, const char* name)
         {
            pod( value, name);
         }

         void Writer::write( const int value, const char* name)
         {
            pod( static_cast< long>( value), name);
         }

         //void write (const unsigned int& value);

         void Writer::write( const long value, const char* name)
         {
            pod( value, name);
         }

         void Writer::write( const long long value, const char* name)
         {
            pod( value, name);
         }

         void Writer::write (const unsigned long value, const char* name)
         {
            pod( static_cast< long>( value), name);
         }

         void Writer::write( const float value, const char* name)
         {
            pod( value, name);
         }

         void Writer::write( const double value, const char* name)
         {
            pod( value, name);
         }

         void Writer::write( const std::string& value, const char* name)
         {
            pod( value, name);
         }

         void Writer::write( const platform::binary_type& value, const char* name)
         {
            pod( value, name);
         }


      } // archive
   } // sf
} // casual

