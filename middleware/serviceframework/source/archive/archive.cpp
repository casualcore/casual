//!
//! casual
//!

#include "sf/archive/archive.h"

#include "common/exception/handle.h"

namespace casual
{
   namespace sf
   {
      namespace archive
      {
         

         Reader::~Reader() = default;

         Reader::Reader( Reader&&) noexcept = default;
         Reader& Reader::operator = ( Reader&&) noexcept = default;

         bool Reader::read( int& value, const char* name)
         {
            long temp;
            if( read( temp, name))
            {
               value = temp;
               return true;
            }
            return false;
         }

         bool Reader::read (unsigned long& value, const char* name)
         {
            long temp;
            if( read( temp, name))
            {
               value = temp;
               return true;
            }
            return false;
         }



         Writer::~Writer()
         {
            try
            {
               if( m_protocol)
                  m_protocol->flush();
            }
            catch( ...)
            {
               common::exception::handle();
            }
         }

         Writer::Writer( Writer&&) noexcept = default;
         Writer& Writer::operator = ( Writer&&) noexcept = default;


         void Writer::write( const int value, const char* name)
         {
            write( static_cast< long>( value), name);
         }


         void Writer::write (const unsigned long value, const char* name)
         {
            write( static_cast< long>( value), name);
         }

      } // archive
   } // sf
} // casual

