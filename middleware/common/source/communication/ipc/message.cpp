//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/communication/ipc/message.h"

#include "common/log.h"

namespace casual
{
   namespace common::communication::ipc::message
   {
      namespace transport
      {
         std::ostream& operator << ( std::ostream& out, const Header& value)
         {
            return stream::write( out, "{ type: ", description( value.type), ", correlation: ", transcode::hex::stream::wrapper( value.correlation),
               ", offset: ", value.offset, ", count: ", value.count, ", size: ", value.size, '}');
         }
         
         static_assert( max_message_size() <= platform::ipc::transport::size, "ipc message is too big'");
         static_assert( header_size() * 10  <  max_message_size(), "header ratio is to big");
      } // transport
      

      std::ostream& operator << ( std::ostream& out, const Transport& value)
      {
         return stream::write( out, 
            "{ header: " , value.message.header
            , ", payload.size: " , value.payload_size()
            , ", header-size: " , transport::header_size()
            , ", transport-size: " ,  value.size()
            , ", max-size: " , transport::max_message_size() 
            , '}');
      }

      Complete::Complete( common::message::Type type, const strong::correlation::id& correlation) : m_type{ type}, m_correlation{ correlation} {}

      Complete::Complete( common::message::Type type, const strong::correlation::id& correlation, platform::binary::type&& payload)
         : payload{ std::move( payload)}, m_type{ type}, m_correlation{ correlation}, m_offset{ Complete::size()} {}

      void Complete::add( const Transport& transport)
      {
         const auto size = std::distance( std::begin( transport), std::end( transport));

         // Transport messsages can be resend, hence we need to ignore the ones that has 
         // a smaller offset than current offset.
         if( transport.payload_offset() < m_offset)
            return;

         if( transport.payload_offset() > m_offset)
            code::raise::error( code::casual::internal_out_of_bounds, "added transport has too large offset: ", transport.payload_offset(), " - current offset: ", m_offset);

         // Some sanity checks
         if( transport.payload_offset() + size > Complete::size())
            code::raise::error( code::casual::internal_out_of_bounds, "added transport is out of bounds: ", transport);

         algorithm::copy( transport, std::begin( payload) + m_offset);
         m_offset += size;
      }

      Complete::operator bool() const
      {
         return type() != common::message::Type::absent_message;
      }

      bool Complete::complete() const noexcept { return static_cast< platform::size::type>( payload.size()) == m_offset;}

   
      strong::execution::id Complete::execution() const noexcept
      {
         if( payload.size() > 16)
            return {};

         Uuid::uuid_type uuid{};
         algorithm::copy_max( payload, uuid);

         return strong::execution::id{ uuid};
      }

      std::ostream& operator << ( std::ostream& out, const Complete& value)
      {
         return stream::write( out, "{ type: ", value.type(), ", correlation: ", value.correlation(), ", size: ",
            value.payload.size(), ", offset: ", value.m_offset, ", complete: ", value.complete());
      }

      static_assert( concepts::nothrow::movable< Complete>, "not movable");

   } // common::communication::ipc::message

} // casual
