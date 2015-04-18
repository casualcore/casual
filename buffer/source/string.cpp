//!
//! string.cpp
//!
//! Created on: Dec 26, 2013
//!     Author: Lazan
//!

#include "buffer/string.h"

#include "common/buffer/pool.h"
#include "common/buffer/type.h"

#include <cstring>

namespace casual
{
   namespace buffer
   {
      namespace string
      {

         namespace
         {

            struct Buffer : common::buffer::Buffer
            {
               using common::buffer::Buffer::Buffer;

               typedef common::platform::binary_type::size_type size_type;

               //
               // TODO: This should be moved to the Allocator-interface
               //
               size_type size( const size_type user_size) const
               {
                  //
                  // Ignore user provided size and return size of string + null
                  //
                  return std::strlen( payload.memory.data()) + 1;
               }

            };



            class Allocator : public common::buffer::pool::basic_pool<Buffer>
            {
            public:

               using types_type = common::buffer::pool::default_pool::types_type;

               static const types_type& types()
               {
                  // The types this pool can manage
                  static const types_type result{{ CASUAL_STRING, "" }};
                  return result;
               }

               common::platform::raw_buffer_type allocate( const common::buffer::Type& type, const common::platform::binary_size_type size)
               {
                  m_pool.emplace_back( type, size > 0 ? size : 1);

                  m_pool.back().payload.memory.front() = '\0';

                  return m_pool.back().payload.memory.data();
               }

               common::platform::raw_buffer_type reallocate( const common::platform::const_raw_buffer_type handle, const common::platform::binary_size_type size)
               {
                  const auto result = find( handle);

                  result->payload.memory.resize( size > 0 ? size : 1);

                  result->payload.memory.back() = '\0';

                  return result->payload.memory.data();
               }
            };

         } //

      } // string

   } // buffer


   //
   // Register and define the type that can be used to get the custom pool
   //
   template class common::buffer::pool::Registration< buffer::string::Allocator>;

   namespace buffer
   {
      namespace string
      {
         using pool_type = common::buffer::pool::Registration< Allocator>;

         namespace
         {

            Buffer* find_buffer( const char* const handle)
            {
               try
               {
                  auto& buffer = pool_type::pool.get( handle);

                  return &buffer;
               }
               catch( ...)
               {
                  //
                  // TODO: Perhaps have some dedicated string-logging ?
                  //
                  common::error::handler();
               }

               return nullptr;

            }
         }

      } // string

   } // buffer

} // casual

const char* CasualStringDescription( const int code)
{

   switch( code)
   {
      case CASUAL_STRING_SUCCESS:
         return "Success";
      case CASUAL_STRING_NO_SPACE:
         return "No space";
      case CASUAL_STRING_NO_PLACE:
         return "No place";
      case CASUAL_STRING_INVALID_BUFFER:
         return "Invalid buffer";
      case CASUAL_STRING_INVALID_ARGUMENT:
         return "Invalid argument";
      case CASUAL_STRING_INTERNAL_FAILURE:
         return "Internal failure";
      default:
         return "Unknown code";
   }

}

int CasualStringExploreBuffer( const char* const handle, long* const size, long* const used)
{
   const auto buffer = casual::buffer::string::find_buffer( handle);

   if( buffer)
   {
      const auto reserved = buffer->payload.memory.size();
      const auto utilized = std::strlen( buffer->payload.memory.data()) + 1;

      if( size) *size = static_cast<long>(reserved);
      if( used) *used = static_cast<long>(utilized);

      if( utilized > reserved)
      {
         // We need to report this
         return CASUAL_STRING_NO_PLACE;
      }

   }
   else
   {
      return CASUAL_STRING_INVALID_BUFFER;
   }

   return CASUAL_STRING_SUCCESS;
}

int CasualStringWriteString( char* const handle, const char* const value)
{
   auto buffer = casual::buffer::string::find_buffer( handle);

   if( buffer)
   {
      if( value)
      {
         const auto count = std::strlen( value) + 1;

         if( count > buffer->payload.memory.size())
         {
            return CASUAL_STRING_NO_SPACE;
         }
         else
         {
            std::memcpy( buffer->payload.memory.data(), value, count);
         }

      }
      else
      {
         return CASUAL_STRING_INVALID_ARGUMENT;
      }

   }
   else
   {
      return CASUAL_STRING_INVALID_BUFFER;
   }

   return CASUAL_STRING_SUCCESS;

}

int CasualStringParseString( const char* handle, const char** value)
{
   const auto buffer = casual::buffer::string::find_buffer( handle);

   if( buffer)
   {
      if( value)
      {
         const auto count = std::strlen( buffer->payload.memory.data()) + 1;

         if( count > buffer->payload.memory.size())
         {
            return CASUAL_STRING_NO_PLACE;
         }
         else
         {
            *value = buffer->payload.memory.data();
         }

      }
      else
      {
         return CASUAL_STRING_INVALID_ARGUMENT;
      }

   }
   else
   {
      return CASUAL_STRING_INVALID_BUFFER;
   }

   return CASUAL_STRING_SUCCESS;

}

