//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "casual/platform.h"

#include "common/serialize/macro.h"
#include "common/algorithm.h"
#include "common/binary/span.h"
#include "common/network/byteorder.h"
#include "common/string.h"
#include "common/strong/type.h"

#include "casual/xatmi/extended.h"
#include "casual/xatmi/defines.h"

#include <string>
#include <ostream>

// nullptr_t
#include <cstddef>

namespace casual
{
   namespace common::buffer
   {
      namespace handle
      {
         namespace detail
         {
            template< typename T>
            struct policy
            {
               constexpr static auto initialize() noexcept { return nullptr;}
               constexpr static bool valid( T value) noexcept { return value != initialize();}
               inline static std::ostream& stream( std::ostream& out, T value) { return stream::write( out, '@', static_cast< const void*>( value));}
            };

         } // detail

         namespace mutate
         {
            using base_type = common::strong::Type< platform::binary::pointer, detail::policy< platform::binary::pointer>>;
            struct type : base_type
            {
               using base_type::base_type;
               explicit type( platform::buffer::raw::type raw) : base_type{ reinterpret_cast< platform::binary::pointer>( raw) } {}

               inline auto raw() { return reinterpret_cast< platform::buffer::raw::type>( value());}


               inline friend bool operator == ( type lhs, mutate::type rhs) { return lhs.value() == rhs.value();}
            };

         } // mutate


         using base_type = common::strong::Type< platform::binary::immutable::pointer, detail::policy< platform::binary::immutable::pointer>>;
         struct type : base_type
         {
            using base_type::base_type;
            //! implicit conversion from _mutable handle type_
            type( mutate::type other) : base_type{ other.value()} {}

            explicit type( platform::buffer::raw::immutable::type raw) : base_type{ reinterpret_cast< platform::binary::immutable::pointer>( raw) } {}

            auto raw() const { return reinterpret_cast< platform::buffer::raw::immutable::type>( value());}



            inline friend bool operator == ( type lhs, mutate::type rhs) { return lhs.value() == rhs.value();}
         };
         
         
      } // handle

      namespace type
      {
         constexpr std::string_view x_octet = X_OCTET "/";

         constexpr std::string_view binary = CASUAL_BUFFER_BINARY_TYPE "/" CASUAL_BUFFER_BINARY_SUBTYPE;
         constexpr std::string_view json = CASUAL_BUFFER_JSON_TYPE "/" CASUAL_BUFFER_JSON_SUBTYPE;
         constexpr std::string_view yaml = CASUAL_BUFFER_YAML_TYPE "/" CASUAL_BUFFER_YAML_SUBTYPE;
         constexpr std::string_view xml = CASUAL_BUFFER_XML_TYPE "/" CASUAL_BUFFER_XML_SUBTYPE;
         constexpr std::string_view ini = CASUAL_BUFFER_INI_TYPE "/" CASUAL_BUFFER_INI_SUBTYPE;

         //! Represent _null_. Not the most elegant solution, but it gives us deterministic deduction
         //! between explicit "NULL" from the "user", and absent buffer.
         constexpr std::string_view null = "NULL";

         std::string combine( const char* type, const char* subtype = nullptr);

         inline auto dismantle( const std::string& type)
         {
            return algorithm::split( type, '/');
         }
      } // type

      struct Payload : compare::Equality< Payload>
      {
         Payload();
         Payload( std::nullptr_t);
         Payload( string::Argument type);
         Payload( string::Argument type, platform::binary::type buffer);
         Payload( string::Argument type, platform::binary::size::type size);
         
         // TODO make sure this type is move only, and remove the copy during _service-forward_
         // Payload( Payload&&) noexcept = default;
         // Payload& operator = ( Payload&&) noexcept = default;

         bool null() const;
         inline explicit operator bool () const { return ! type.empty();}

         inline buffer::handle::type handle() const noexcept { return buffer::handle::type{ data.data()};}
         inline buffer::handle::mutate::type handle() noexcept { return buffer::handle::mutate::type{ data.data()};}

         std::string type;
         platform::binary::type data;

         inline auto tie() const noexcept { return std::tie( type, data);}

         friend std::ostream& operator << ( std::ostream& out, const Payload& value);

         CASUAL_CONST_CORRECT_SERIALIZE(
            CASUAL_SERIALIZE( type);
            CASUAL_SERIALIZE( data);
         )
      };


      namespace payload
      {

         struct Send
         {
            using size_type = platform::binary::size::type;

            inline Send( const Payload& payload, size_type transport, size_type reserved)
               :  m_payload( payload), m_transport{ transport}, m_reserved{ reserved} {}

            inline Send( const Payload& payload)
               : m_payload( payload), m_transport( payload.data.size()) {}

            inline const Payload& payload() const noexcept { return m_payload.get();};

            inline auto transport() const noexcept { return m_transport;}
            inline auto reserved() const noexcept { return m_reserved;}

            // We mimic buffer::Payload and only send 'transport' portion of memory
            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE_NAME( payload().type, "type");
               CASUAL_SERIALIZE_NAME( m_transport, "size"); // size of the memory
               CASUAL_SERIALIZE_NAME( binary::span::fixed::make( std::begin( payload().data), m_transport), "memory");
            )

            friend std::ostream& operator << ( std::ostream& out, const Send& value);

         private:
            std::reference_wrapper< const Payload> m_payload;
            size_type m_transport = 0;
            size_type m_reserved = 0;
         };

      } // payload

      struct Buffer
      {
         // A tag for construction to indicate that the buffer is an "inbound"
         struct Inbound {};

         Buffer( Payload payload);
         //! construct an "inbound buffer".
         Buffer( Payload payload, Inbound);
         Buffer( string::Argument type, platform::binary::size::type size);

         Buffer( Buffer&&) noexcept;
         Buffer& operator = ( Buffer&&) noexcept;

         platform::binary::size::type transport( platform::binary::size::type user_size) const;
         platform::binary::size::type reserved() const;

         inline buffer::handle::type handle() const noexcept { return payload.handle();}
         inline buffer::handle::mutate::type handle() noexcept { return payload.handle();}

         bool inbound() const { return m_inbound;}

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( payload);
         )

         Payload payload;
      private:
         bool m_inbound = false;
      };

   } // common::buffer
} // casual


