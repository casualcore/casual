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
            namespace local
            {
               namespace
               {
                  namespace implementation
                  {
                     using size_type = common::platform::size::type;

                     struct Writer : native::binary::Output
                     {
                        using base_type = native::binary::Output;
                        using base_type::base_type;


                        //! @{
                        //! No op
                        void container_end( const char*) { /*no op*/}
                        void composite_start( const char*) {  /*no op*/}
                        void composite_end(  const char*) { /*no op*/}
                        //! @}


                        size_type container_start( const size_type size, const char*)
                        {
                           write( size, nullptr);
                           return size;
                        }
                     };


                     struct Reader : native::binary::Input
                     {
                        using base_type = native::binary::Input;
                        using base_type::base_type;

                        //! @{
                        //! No op
                        void container_end( const char*) { /*no op*/}
                        void composite_end(  const char*) { /*no op*/}
                        //! @}

                        bool composite_start( const char*) { return true;}

                        std::tuple< size_type, bool> container_start( size_type size, const char*)
                        {
                           read( size, nullptr);
                           return std::make_tuple( size, true);
                        }

                        template< typename T> 
                        bool read( T& value, const char*)
                        {
                           base_type::read( value, nullptr);
                           return true;
                        }
                     };
                  } // implementation

               } // <unnamed>
            } // local

            serialize::Reader reader( const common::platform::binary::type& source)
            {
               return serialize::Reader::emplace< serialize::policy::Relaxed< local::implementation::Reader>>( source);
            }

            serialize::Writer writer( common::platform::binary::type& destination)
            {
               return serialize::Writer::emplace< local::implementation::Writer>( destination);
            }

         } // binary
      } // serialize
   } // common
} // casual