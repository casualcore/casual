//!
//! type.cpp
//!
//! Created on: Feb 22, 2015
//!     Author: Lazan
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
         Type::Type( std::string type, std::string subtype) : type( std::move( type)), subtype( std::move( subtype)) {}
         Type::Type( const char* type, const char* subtype) : type( type ? type : ""), subtype( subtype ? subtype : "") {}


         bool operator < ( const Type& lhs, const Type& rhs)
         {
            if( lhs.type < rhs.type)
               return true;
            if( rhs.type < lhs.type)
               return false;
            return lhs.subtype < rhs.subtype;
         }

         bool operator == ( const Type& lhs, const Type& rhs)
         {
            return lhs.type == rhs.type && lhs.subtype == rhs.subtype;
         }

         bool operator != ( const Type& lhs, const Type& rhs)
         {
            return ! ( lhs == rhs);
         }

         std::ostream& operator << ( std::ostream& out, const Type& value)
         {
            return out << "{type: " << value.type << " subtype: " << value.subtype << "}";
         }



         Payload::Payload() = default;

         Payload::Payload( std::nullptr_t) : type{ "NULL", ""} {}

         Payload::Payload( Type type, platform::binary_type buffer)
          : type( std::move( type)), memory( std::move( buffer)) {}

         Payload::Payload( Type type, platform::binary_type::size_type size)
          : type( std::move( type)), memory( size) {}


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




         Buffer::Buffer( Payload payload) : payload( std::move( payload)) {}

         Buffer::Buffer( Type type, platform::binary_type::size_type size)
             : payload( std::move( type), size) {}


         Buffer::Buffer( Buffer&&) noexcept = default;
         Buffer& Buffer::operator = ( Buffer&&) noexcept = default;

         platform::binary_type::size_type Buffer::size( platform::binary_type::size_type user_size) const
         {
            if( user_size > payload.memory.size())
            {
               throw exception::xatmi::InvalidArguments{ "user supplied size is larger than the buffer actual size"};
            }

            return user_size;
         }


      } // buffer
   } // common
} // casual
