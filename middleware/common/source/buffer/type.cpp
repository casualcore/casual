//!
//! casual
//!

#include "common/buffer/type.h"

#include "common/exception.h"

namespace casual
{
   namespace common
   {
      namespace buffer
      {

         Type::Type() = default;
         Type::Type( std::string name, std::string subname) : name( std::move( name)), subname( std::move( subname)) {}
         Type::Type( const char* name, const char* subname) : name( name ? name : ""), subname( subname ? subname : "") {}


         bool operator < ( const Type& lhs, const Type& rhs)
         {
            if( lhs.name == rhs.name)
               return lhs.subname < rhs.subname;

            return lhs.name < rhs.name;
         }

         bool operator == ( const Type& lhs, const Type& rhs)
         {
            return lhs.name == rhs.name && lhs.subname == rhs.subname;
         }

         bool operator != ( const Type& lhs, const Type& rhs)
         {
            return ! ( lhs == rhs);
         }

         std::ostream& operator << ( std::ostream& out, const Type& value)
         {
            return out << "{name: " << value.name << " subname: " << value.subname << "}";
         }

         namespace type
         {
            Type x_octet() { return { X_OCTET, nullptr};}

            Type binary() { return { CASUAL_BUFFER_BINARY_TYPE, CASUAL_BUFFER_BINARY_SUBTYPE};}
            Type json() { return { CASUAL_BUFFER_JSON_TYPE, CASUAL_BUFFER_JSON_SUBTYPE};}
            Type yaml() { return { CASUAL_BUFFER_YAML_TYPE, CASUAL_BUFFER_YAML_SUBTYPE};}
            Type xml() { return { CASUAL_BUFFER_XML_TYPE, CASUAL_BUFFER_XML_SUBTYPE};}
            Type ini() { return { CASUAL_BUFFER_INI_TYPE, CASUAL_BUFFER_INI_SUBTYPE};}
         } // type



         Payload::Payload() = default;

         Payload::Payload( std::nullptr_t) : type{ "NULL", ""} {}

         Payload::Payload( Type type, platform::binary_type buffer)
          : type( std::move( type)), memory( std::move( buffer)) {}

         Payload::Payload( Type type, platform::binary_type::size_type size)
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
            return type.name == "NULL";
         }


         std::ostream& operator << ( std::ostream& out, const Payload& value)
         {
            return out << "{ type: " << value.type << ", @" << static_cast< const void*>( value.memory.data()) << " size: " << value.memory.size() << '}';
         }


         namespace payload
         {
            std::ostream& operator << ( std::ostream& out, const Send& value)
            {
               return out << "{ payload: " << value.payload << ", transport: " << value.transport << ", reserved: " << value.reserved <<'}';
            }
         }

         Buffer::Buffer( Payload payload) : payload( std::move( payload)) {}

         Buffer::Buffer( Type type, platform::binary_type::size_type size)
             : payload( std::move( type), size) {}


         Buffer::Buffer( Buffer&&) noexcept = default;
         Buffer& Buffer::operator = ( Buffer&&) noexcept = default;

         platform::binary_type::size_type Buffer::transport( platform::binary_type::size_type user_size) const
         {
            if( user_size > payload.memory.size())
            {
               throw exception::xatmi::invalid::Argument{ "user supplied size is larger than the buffer actual size"};
            }

            return user_size;
         }

         platform::binary_type::size_type Buffer::reserved() const
         {
            return payload.memory.size();
         }

      } // buffer
   } // common
} // casual
