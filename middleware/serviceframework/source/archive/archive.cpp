

#include "sf/archive/archive.h"

namespace casual
{
   namespace sf
   {
      namespace archive
      {


         Reader::Reader() = default;
         Reader::~Reader() = default;



         std::tuple< std::size_t, bool> Reader::container_start( std::size_t size, const char* name)
         {
            return dispatch_container_start( size, name);
         }

         void Reader::container_end( const char* name)
         {
            dispatch_container_end( name);
         }

         bool Reader::serialtype_start( const char* name)
         {
            return dispatch_serialtype_start( name);
         }

         void Reader::serialtype_end( const char* name)
         {
            dispatch_serialtype_end( name);
         }



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

         bool Reader::read( platform::binary::type& value, const char* name)
         {
            return pod( value, name);
         }



         Writer::Writer() = default;
         Writer::~Writer() = default;

         void Writer::container_start( std::size_t size, const char* name)
         {
            return dispatch_container_start( size, name);
         }

         void Writer::container_end( const char* name)
         {
            dispatch_container_end( name);
         }

         void Writer::serialtype_start( const char* name)
         {
            dispatch_serialtype_start( name);
         }

         void Writer::serialtype_end( const char* name)
         {
            dispatch_serialtype_end( name);
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

         void Writer::write( const platform::binary::type& value, const char* name)
         {
            pod( value, name);
         }


      } // archive
   } // sf
} // casual

