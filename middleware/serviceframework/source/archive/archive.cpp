

#include "sf/archive/archive.h"

namespace casual
{
   namespace sf
   {
      namespace archive
      {


         Base::Base() = default;
         Base::~Base() = default;


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



         Reader::Reader() = default;
         Reader::~Reader() = default;

         bool Reader::read( bool& value, const char* name)
         {
            return pod( value, name);
         }

         bool Reader::read( char& value, const char* name)
         {
            return pod( value, name);
         }

         bool Reader::read( short& value, const char* name)
         {
            return pod( value, name);
         }

         bool Reader::read( int& value, const char* name)
         {
            long temp;
            if( pod( temp, name))
            {
               value = temp;
               return true;
            }
            return false;
         }

         //void read (const unsigned int& value);

         bool Reader::read( long& value, const char* name)
         {
            return pod( value, name);
         }

         bool Reader::read( long long& value, const char* name)
         {
            return pod( value, name);
         }

         bool Reader::read (unsigned long& value, const char* name)
         {
            long temp;
            if( pod( temp, name))
            {
               value = temp;
               return true;
            }
            return false;
         }

         bool Reader::read( float& value, const char* name)
         {
            return pod( value, name);
         }

         bool Reader::read( double& value, const char* name)
         {
            return pod( value, name);
         }

         bool Reader::read( std::string& value, const char* name)
         {
            return pod( value, name);
         }

         bool Reader::read( platform::binary_type& value, const char* name)
         {
            return pod( value, name);
         }



         Writer::Writer() = default;
         Writer::~Writer() = default;

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

