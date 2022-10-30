//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/buffer/type.h"
#include "common/serialize/native/network.h"
#include "common/log.h"

#include "common/code/raise.h"
#include "common/code/xatmi.h"

namespace casual
{
   namespace common
   {
      namespace buffer
      {
         namespace type
         {

            std::string combine( const char* type, const char* subtype )
            {
               std::string result{ type};
               result.push_back( '/');

               if( subtype && subtype[ 0] != '\0')
                  return result + subtype;

               return result;
            }


         } // type

         static_assert( traits::is::nothrow::movable_v< Payload>);
         // TODO make sure this type is move only, and remove the copy during _service-forward_
         //static_assert( ! traits::is::copyable_v< Payload>);

         Payload::Payload() = default;

         Payload::Payload( std::nullptr_t) : Payload{ std::string{ "NULL"}, 0} {}

         Payload::Payload( string::Argument type) : type( std::move( type)) {}

         Payload::Payload( string::Argument type, platform::binary::type buffer)
          : type( std::move( type)), data( std::move( buffer)) {}

         Payload::Payload( string::Argument type, platform::binary::size::type size)
          : type( std::move( type)), data( size)
         {
            if( ! data.data())
               data.reserve( 1);
         }

         bool Payload::null() const
         {
            return type == "NULL";
         }

         std::ostream& operator << ( std::ostream& out, const Payload& value)
         {
            return out << "{ type: " << value.type 
               << ", memory: @" << static_cast< const void*>( value.data.data()) 
               << ", size: " << value.data.size() 
               << ", capacity: " << value.data.capacity() 
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

         static_assert( traits::is::nothrow::movable_v< Buffer>);
         static_assert( ! traits::is::copyable_v< Buffer>);

         Buffer::Buffer( Payload payload) : payload( std::move( payload)) {}

         Buffer::Buffer( string::Argument type, platform::binary::size::type size)
            : payload( std::move( type), size) {}


         Buffer::Buffer( Buffer&&) noexcept = default;
         Buffer& Buffer::operator = ( Buffer&&) noexcept = default;

         platform::binary::size::type Buffer::transport( platform::binary::size::type user_size) const
         {
            if( user_size > reserved())
               code::raise::error( code::xatmi::argument, "user supplied size is larger than the buffer actual size");

            return user_size;
         }

         platform::binary::size::type Buffer::reserved() const
         {
            return payload.data.size();
         }


      } // buffer
   } // common
} // casual
