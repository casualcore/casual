//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/serialize/binary.h"
#include "common/serialize/policy.h"
#include "common/serialize/native/binary.h"
#include "common/serialize/create.h"
#include "common/buffer/type.h"

#include "common/memory.h"
#include "common/network/byteorder.h"

namespace casual
{
   namespace common
   {
      namespace serialize::binary
      {
         namespace local
         {
            namespace
            {
               constexpr auto keys() 
               {
                  using namespace std::string_view_literals; 
                  return array::make( "binary"sv, common::buffer::type::binary);
               }

               namespace implementation
               {
                  struct Writer : native::binary::Writer
                  {
                     constexpr static auto archive_properties() { return common::serialize::archive::Property::order;}

                     constexpr static auto keys() { return local::keys();}
                  };


                  struct Reader : native::binary::Reader
                  {
                     constexpr static auto archive_properties() { return common::serialize::archive::Property::order;}

                     Reader( const platform::binary::type& buffer) : native::binary::Reader{ buffer} {}

                     Reader( std::istream& in) : native::binary::Reader{ m_buffer} 
                     {
                        while( in)
                           m_buffer.push_back( in.get());
                     }

                     constexpr static auto keys() { return local::keys();}

                  private:
                     platform::binary::type m_buffer;

                  };
               } // implementation
            } // <unnamed>
         } // local


         serialize::Reader reader( const platform::binary::type& source)
         {
            return serialize::Reader::emplace< native::binary::Reader>( source);
         }

         serialize::Writer writer()
         {
            return serialize::Writer::emplace< native::binary::Writer>();
         }

         namespace create
         {
            namespace reader
            {
               //template struct Registration< binary::local::implementation::Reader>;
            } // writer
            namespace writer
            {
               //template struct Registration< binary::local::implementation::Writer>;
            } // writer
         } // create

      } // serialize::binary
   } // common
 } // casual