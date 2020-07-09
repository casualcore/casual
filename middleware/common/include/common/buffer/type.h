//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "casual/platform.h"

#include "common/serialize/macro.h"
#include "common/algorithm.h"
#include "common/view/binary.h"
#include "common/network/byteorder.h"


#include <string>
#include <ostream>

// nullptr_t
#include <cstddef>

namespace casual
{
   namespace common
   {
      namespace buffer
      {
         namespace type
         {
            const std::string& x_octet();
            const std::string& binary();
            const std::string& json();
            const std::string& yaml();
            const std::string& xml();
            const std::string& ini();

            std::string combine( const char* type, const char* subtype = nullptr);

            inline auto dismantle( const std::string& type)
            {
               return algorithm::split( type, '/');
            }
         } // type

         struct Payload
         {
            Payload();
            Payload( std::nullptr_t);
            Payload( std::string type);
            Payload( std::string type, platform::binary::type buffer);
            Payload( std::string type, platform::binary::size::type size);

            //!
            //! g++ does not generate noexcept move ctor/assignment
            //! @{
            Payload( Payload&& rhs) noexcept;
            Payload& operator = ( Payload&& rhs) noexcept;
            //! @}


            Payload( const Payload&);
            Payload& operator = ( const Payload&);

            bool null() const;
            inline explicit operator bool () const { return ! type.empty();}

            std::string type;
            platform::binary::type memory;

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               CASUAL_SERIALIZE( type);
               CASUAL_SERIALIZE( memory);
            })

            friend std::ostream& operator << ( std::ostream& out, const Payload& value);
         };

         namespace payload
         {

            struct Send
            {
               using size_type = platform::binary::size::type;

               inline Send( const Payload& payload, size_type transport, size_type reserved)
                  :  m_payload( payload), m_transport{ transport}, m_reserved{ reserved} {}

               inline Send( const Payload& payload)
                  : m_payload( payload), m_transport( payload.memory.size()) {}

               inline const Payload& payload() const noexcept { return m_payload.get();};

               inline auto transport() const noexcept { return m_transport;}
               inline auto reserved() const noexcept { return m_reserved;}

               // We mimic buffer::Payload and only send 'transport' portion of memory
               CASUAL_LOG_SERIALIZE(
               {
                  CASUAL_SERIALIZE_NAME( payload().type, "type");
                  CASUAL_SERIALIZE_NAME( m_transport, "size"); // size of the memory
                  CASUAL_SERIALIZE_NAME( view::binary::make( std::begin( payload().memory), m_transport), "memory");
               })

               friend std::ostream& operator << ( std::ostream& out, const Send& value);

            private:
               std::reference_wrapper< const Payload> m_payload;
               size_type m_transport = 0;
               size_type m_reserved = 0;
            };

         } // payload

         struct Buffer
         {
            Buffer( Payload payload);
            Buffer( std::string type, platform::binary::size::type size);

            Buffer( Buffer&&) noexcept;
            Buffer& operator = ( Buffer&&) noexcept;

            Buffer( const Buffer&) = delete;
            Buffer& operator = ( const Buffer&) = delete;

            platform::binary::size::type transport( platform::binary::size::type user_size) const;
            platform::binary::size::type reserved() const;

            CASUAL_LOG_SERIALIZE(
            {
               CASUAL_SERIALIZE( payload);
            })

            Payload payload;
         };

      } // buffer
   } // common

} // casual


