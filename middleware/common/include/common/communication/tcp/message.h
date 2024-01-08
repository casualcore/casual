//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "casual/platform.h"

#include "common/serialize/native/complete.h"
#include "common/serialize/native/network.h"
#include "common/message/type.h"
#include "common/uuid.h"
#include "common/network/byteorder.h"
#include "common/serialize/macro.h"

#include <iosfwd>

namespace casual
{
   namespace common::communication::tcp::message
   {
      struct Header
      {
         using host_type_type = platform::ipc::message::type;
         using network_type_type = common::network::byteorder::type<host_type_type>;

         using host_uuid_type = Uuid::uuid_type;
         using network_uuid_type = host_uuid_type;

         using host_size_type = platform::size::type;
         using network_size_type = common::network::byteorder::type<host_size_type>;

         network_type_type type{};
         network_uuid_type correlation{};
         network_size_type size{};

         static_assert( sizeof( network_type_type) ==  8, "Wrong size for type");
         static_assert( sizeof( network_uuid_type) == 16, "Wrong size for uuid");
         static_assert( sizeof( network_size_type) ==  8, "Wrong size for size");


         friend std::ostream& operator << ( std::ostream& out, const Header& value);


         CASUAL_CONST_CORRECT_SERIALIZE(
            CASUAL_SERIALIZE( type);
            CASUAL_SERIALIZE( correlation);
            CASUAL_SERIALIZE( size);
         )
      };

      static_assert( std::is_trivially_copyable< Header>::value, "tcp::message::Header needs to be trivially copyable" );
      
      namespace header
      {
         inline constexpr platform::size::type size = sizeof( Header);


         namespace detail
         {
            inline auto type( const Header& header) noexcept
            {
               return static_cast< common::message::Type>( common::network::byteorder::decode< Header::host_type_type>( header.type));
            }

            inline auto size( const Header& header) noexcept 
            {
               return common::network::byteorder::size::decode< Header::host_size_type>( header.size);
            }

            inline auto correlation( const Header& header)
            {
               return strong::correlation::id::emplace( header.correlation);
            }
            
         } // detail
      } // header

      struct Complete
      {
         using payload_type = platform::binary::type;
         using range_type = range::type_t< payload_type>;

         Complete() = default;
         Complete( common::message::Type type, strong::correlation::id correlation, payload_type payload);

         //! this is always in network byteordering, 

         platform::size::type offset{};
         payload_type payload;

         //! header functions
         //! @{
         inline auto type() const noexcept { return header::detail::type( m_header);}
         inline auto size() const noexcept { return header::detail::size( m_header);}
         inline auto correlation() const noexcept { return header::detail::correlation( m_header);}
         //! @}

         //! @returns the extracted mandatory execution id from the payload
         strong::execution::id execution() const noexcept;

         inline const char* header_data() const noexcept { return reinterpret_cast< const char*>( &m_header);}
         inline char* header_data() noexcept { return reinterpret_cast< char*>( &m_header);}

         inline auto empty() const noexcept { return offset == 0;}
         
         bool complete() const noexcept;
         
         inline explicit operator bool() const noexcept { return complete();}

         friend std::ostream& operator << ( std::ostream& out, const Complete& value);

      private:
         message::Header m_header{};
      };

   } // common::communication::tcp::message

   namespace common::serialize::native::customization
   {
      template<>
      struct point< communication::tcp::message::Complete>
      {
         using writer = binary::network::create::Writer;
         using reader = binary::network::create::Reader;
      };

   } //common::serialize::native::customization

} // casual


