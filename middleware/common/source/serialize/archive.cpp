//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/serialize/archive.h"

#include "common/exception/handle.h"

namespace casual
{
   namespace common
   {
      namespace serialize
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

         Writer::~Writer() = default;

         Writer::Writer( Writer&&) noexcept = default;
         Writer& Writer::operator = ( Writer&&) noexcept = default;

         void Writer::save( const int value, const char* name)
         {
            save( static_cast< long>( value), name);
         }

         void Writer::save( const unsigned long value, const char* name)
         {
            save( static_cast< long>( value), name);
         }

      } // serialize
   } // common
} // casual

