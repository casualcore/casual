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
         return stream::write( out, "{ type: ", local::host::header::type( value),
            ", correlation: ", transcode::hex::stream::wrapper( value.correlation),
            ", size: ", local::host::header::size( value), '}');

      }

      Complete::Complete( common::message::Type type, strong::correlation::id correlation, payload_type payload)
         : payload{ std::move( payload)}
      {
         m_header.type = network::byteorder::encode( cast::underlying( type));
         correlation.underlaying().copy( m_header.correlation);
         m_header.size = network::byteorder::size::encode( Complete::payload.size());
      }

      bool Complete::complete() const noexcept 
      { 
         return type() != common::message::Type::absent_message 
            && offset == message::header::size + range::size( payload);
      }

      std::ostream& operator << ( std::ostream& out, const Complete& value)
      {
         return out << "{ header: " << value.m_header 
            << ", size: "<< value.payload.size() 
            << ", offset: " << value.offset 
            << ", total: " << value.payload.size() + message::header::size
            << std::boolalpha << ", complete: " << value.complete() << '}';
      }

   } // common::communication::ipc::message

} // casual
