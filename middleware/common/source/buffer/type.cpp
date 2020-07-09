//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/buffer/type.h"
#include "common/exception/xatmi.h"
#include "common/serialize/native/network.h"
#include "common/log.h"

#include "casual/xatmi/extended.h"
#include "casual/xatmi/defines.h"

namespace casual
{
   namespace common
   {
      namespace buffer
      {
         namespace type
         {
            const std::string& x_octet() { static const auto name = combine( X_OCTET); return name;}

            const std::string& binary() { static const auto name = combine( CASUAL_BUFFER_BINARY_TYPE, CASUAL_BUFFER_BINARY_SUBTYPE); return name;}
            const std::string& json() { static const auto name = combine( CASUAL_BUFFER_JSON_TYPE, CASUAL_BUFFER_JSON_SUBTYPE); return name;}
            const std::string& yaml() { static const auto name = combine( CASUAL_BUFFER_YAML_TYPE, CASUAL_BUFFER_YAML_SUBTYPE); return name;}
            const std::string& xml() { static const auto name = combine( CASUAL_BUFFER_XML_TYPE, CASUAL_BUFFER_XML_SUBTYPE); return name;}
            const std::string& ini() { static const auto name = combine( CASUAL_BUFFER_INI_TYPE, CASUAL_BUFFER_INI_SUBTYPE); return name;}


            std::string combine( const char* type, const char* subtype )
            {
               std::string result{ type};
               result.push_back( '/');

               if( subtype && subtype[ 0] != '\0') { return result + subtype;}

               return result;
            }


         } // type



         Payload::Payload() = default;

         Payload::Payload( std::nullptr_t) : Payload{ "NULL", 0} {}

         Payload::Payload( std::string type) : type( std::move( type)) {}

         Payload::Payload( std::string type, platform::binary::type buffer)
          : type( std::move( type)), memory( std::move( buffer)) {}

         Payload::Payload( std::string type, platform::binary::size::type size)
          : type( std::move( type)), memory( size)
         {
            if( ! memory.data())
            {
               memory.reserve( 1);
            }
         }


         Payload::Payload( Payload&& rhs) noexcept
         {
            type = std::move( rhs.type);
            memory = std::move( rhs.memory);
         }
         Payload& Payload::operator = ( Payload&& rhs) noexcept
         {
            type = std::move( rhs.type);
            memory = std::move( rhs.memory);
            return *this;
         }


         Payload::Payload( const Payload&)  = default;
         Payload& Payload::operator = ( const Payload&) = default;

         bool Payload::null() const
         {
            return type == "NULL";
         }

         std::ostream& operator << ( std::ostream& out, const Payload& value)
         {
            return out << "{ type: " << value.type 
               << ", memory: " << static_cast< const void*>( value.memory.data()) 
               << ", size: " << value.memory.size() 
               << ", capacity: " << value.memory.capacity() 
               << '}';
         }

         namespace payload
         {
            std::ostream& operator << ( std::ostream& out, const Send& value)
            {
               return out << "{ payload: " << value.payload() 
                  << ", transport: " << value.transport()
                  <<'}';
            }
         }

         Buffer::Buffer( Payload payload) : payload( std::move( payload)) {}

         Buffer::Buffer( std::string type, platform::binary::size::type size)
             : payload( std::move( type), size) {}


         Buffer::Buffer( Buffer&&) noexcept = default;
         Buffer& Buffer::operator = ( Buffer&&) noexcept = default;

         platform::binary::size::type Buffer::transport( platform::binary::size::type user_size) const
         {
            if( user_size > reserved())
               throw exception::xatmi::invalid::Argument{ "user supplied size is larger than the buffer actual size"};

            return user_size;
         }


         platform::binary::size::type Buffer::reserved() const
         {
            return payload.memory.size();
         }


      } // buffer
   } // common
} // casual
