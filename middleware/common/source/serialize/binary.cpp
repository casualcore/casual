//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/serialize/binary.h"
#include "common/serialize/policy.h"
#include "common/serialize/native/binary.h"

#include "common/memory.h"
#include "common/network/byteorder.h"

namespace casual
{
   namespace common
   {
      namespace serialize
      {
         namespace binary
         {
            /*
            namespace local
            {
               namespace
               {
                  namespace implementation
                  {
                     using size_type = common::platform::size::type;

                     using writer_base = native::binary::Writer;
                     struct Writer : writer_base
                     {
                        using writer_base::writer_base;

                        size_type container_start( const size_type size, const char*)
                        {
                           write( size, nullptr);
                           return size;
                        }
                     };

                     using reader_base = native::binary::Reader;
                     struct Reader : reader_base
                     {
                        using reader_base::reader_base;

                        bool composite_start( const char*) { return true;}

                        std::tuple< size_type, bool> container_start( size_type size, const char*)
                        {
                           read( size, nullptr);
                           return std::make_tuple( size, true);
                        }

                        template< typename T> 
                        bool read( T& value, const char*)
                        {
                           reader_base::read( value, nullptr);
                           return true;
                        }
                     };
                  } // implementation
               } // <unnamed>
            } // local
            */

            serialize::Reader reader( const common::platform::binary::type& source)
            {
               return serialize::Reader::emplace< native::binary::Reader>( source);
            }

            serialize::Writer writer( common::platform::binary::type& destination)
            {
               return serialize::Writer::emplace< native::binary::Writer>( destination);
            }

         } // binary
      } // serialize
   } // common
} // casual