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

         network_type_type type = 0;
         network_uuid_type correlation;
         network_size_type size = 0;

         static_assert( sizeof( network_type_type) ==  8, "Wrong size for type");
         static_assert( sizeof( network_uuid_type) == 16, "Wrong size for uuid");
         static_assert( sizeof( network_size_type) ==  8, "Wrong size for size");

         inline friend constexpr platform::size::type size( const Header&) { return sizeof( Header);};

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
      } // header

      struct Complete
      {
         using payload_type = platform::binary::type;
         using range_type = range::type_t< payload_type>;

         Complete() = default;
         Complete( const message::Header& header);
         inline Complete( common::message::Type type, Uuid correlation, payload_type payload)
            : type{ type}, correlation{ std::move( correlation)}, payload{ std::move( payload)} {}

         common::message::Type type{};
         Uuid correlation;
         platform::size::type offset{};
         payload_type payload;

         message::Header header() const noexcept;

         inline auto empty() const noexcept { return offset == 0;}
         inline platform::size::type size() const noexcept { return payload.size();}
         bool complete() const noexcept;
         
         inline explicit operator bool() const noexcept { return complete();}

         friend std::ostream& operator << ( std::ostream& out, const Complete& value);
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


