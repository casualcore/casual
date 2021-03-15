//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include "common/communication/tcp/message.h"
#include "common/communication/log.h"

namespace casual
{
   namespace common::communication::tcp::message
   {
      namespace local
      {
         namespace
         {
            namespace host::header
            {
               auto type( const Header& value)
               {
                  return static_cast< common::message::Type>( common::network::byteorder::decode< Header::host_type_type>( value.type));
               }

               auto size( const Header& value)
               {
                  return common::network::byteorder::size::decode< Header::host_size_type>( value.size);
               }

            } // host::header
            
         } // <unnamed>
      } // local

      std::ostream& operator << ( std::ostream& out, const Header& value)
      {
         return out << "{ type: " << local::host::header::type( value)
            << ", correlation: " << common::Uuid{ value.correlation}
            << ", size: " << local::host::header::size( value)
            << '}';

      }

      Complete::Complete( const message::Header& header)
         : type{ local::host::header::type( header)}, correlation{ header.correlation}, offset{ header::size}
      {
         log::line( verbose::log, "size: ", local::host::header::size( header));
         payload.resize( local::host::header::size( header));
      }

      message::Header Complete::header() const noexcept
      {
         Header header;
         correlation.copy( header.correlation);
         header.type = network::byteorder::encode( cast::underlying( type));
         header.size = network::byteorder::size::encode( payload.size());
         return header;
      }

      bool Complete::complete() const noexcept 
      { 
         return type != common::message::Type::absent_message 
            && offset == message::header::size + range::size( payload);
      }

      std::ostream& operator << ( std::ostream& out, const Complete& value)
      {
         return out << "{ type: " << value.type << ", correlation: " << value.correlation << ", size: "
            << value.payload.size() << std::boolalpha << ", complete: " << value.complete() << '}';
      }

   } // common::communication::ipc::message

} // casual
