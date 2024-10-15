//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "casual/buffer/string.h"

#include "common/buffer/pool.h"
#include "common/buffer/type.h"

#include "common/log.h"
#include "common/memory.h"
#include "common/exception/capture.h"

#include "common/code/raise.h"
#include "common/code/xatmi.h"


#include <cstring>

namespace casual
{
   namespace buffer
   {
      namespace string
      {

         namespace
         {
            using size_type = platform::size::type;
            using data_type = platform::buffer::raw::type;

            namespace local
            {
               auto validate( const common::buffer::Payload& payload)
               {
                  auto string_like = common::binary::span::to_string_like( payload.data);
                  const auto size = std::size( string_like);
                  const auto used = std::strlen( string_like.data()) + 1;

                  // We do need to check that it is a real null-terminated
                  // string within allocated area

                  if( used > size)
                     throw std::invalid_argument{"string is longer than allocated size"};

                  return used;
               }
            } // local


            struct Buffer : common::buffer::Buffer
            {
               using common::buffer::Buffer::Buffer;

               //! Implement Buffer::transport
               platform::binary::size::type transport( const platform::binary::size::type user_size) const
               {
                  // Just ignore user-size all together
                  //
                  // ... but we do need to validate it
                  return local::validate( payload);
               }

            };

            using allocator_base = common::buffer::pool::implementation::Default< Buffer>;
            struct Allocator : allocator_base
            {
               static constexpr auto types() 
               {
                  return common::array::make( key);
               };

               common::buffer::handle::mutate::type allocate( std::string_view type, platform::binary::size::type size)
               {
                  auto& buffer = allocator_base::emplace_back( type, size ? size : 1);

                  buffer.payload.data.front() = std::byte{ '\0'};

                  return buffer.payload.handle();
               }

               common::buffer::handle::mutate::type reallocate( common::buffer::handle::type handle, platform::binary::size::type size)
               {
                  auto& buffer = allocator_base::get( handle);

                  buffer.payload.data.resize( size ? size : 1);
                  buffer.payload.data.back() = std::byte{ '\0'};

                  // Allow user to reduce allocation
                  buffer.payload.data.shrink_to_fit();

                  return buffer.payload.handle();
               }

               auto& insert( common::buffer::Payload payload)
               {
                  // Validate it before we move it
                  local::validate( payload);

                  return allocator_base::emplace_back( std::move( payload));
               }

            };

            using pool_type = common::buffer::pool::Registration< Allocator>;

            int error() noexcept
            {
               try
               {
                  throw;
               }
               catch( const std::out_of_range&)
               {
                  return CASUAL_STRING_OUT_OF_BOUNDS;
               }
               catch( const std::invalid_argument&)
               {
                  return CASUAL_STRING_INVALID_ARGUMENT;
               }
               catch( const std::bad_alloc&)
               {
                  return CASUAL_STRING_OUT_OF_MEMORY;
               }
               catch( ...)
               {
                  const auto error = common::exception::capture();

                  if( error.code() == common::code::xatmi::argument)
                     return CASUAL_STRING_INVALID_HANDLE; 

                  common::log::line( common::log::category::error, error);

                  return CASUAL_STRING_INTERNAL_FAILURE;
               }
            }


            int explore( const char* const handle, size_type* const size, size_type* const used)
            {
               try
               {
                  const auto& buffer = pool_type::pool().get( common::buffer::handle::type{ handle});

                  if( size) 
                     *size = buffer.payload.data.size();
                  if( used)
                  {
                     if( auto found = common::algorithm::find( buffer.payload.data, std::byte{ '\0'}))
                        *used = std::distance( std::begin( buffer.payload.data), std::begin( found)) + 1;
                     else
                        return CASUAL_STRING_INTERNAL_FAILURE;
                  }
               }
               catch( ...)
               {
                  return error();
               }

               return CASUAL_STRING_SUCCESS;
            }


            int set( char** const handle, const char* const value)
            {
               try
               {
                  auto& buffer = pool_type::pool().get( common::buffer::handle::type{ *handle});

                  const auto count = std::strlen( value) + 1;

                  casual::common::algorithm::copy( common::binary::span::make( value, count), buffer.payload.data);

                  *handle = buffer.payload.handle().raw();
               }
               catch( ...)
               {
                  return error();
               }

               return CASUAL_STRING_SUCCESS;

            }

            int get( const char* const handle, const char*& value)
            {
               try
               {
                  const auto& buffer = pool_type::pool().get( common::buffer::handle::type{ handle});

                  value = reinterpret_cast< const char*>( buffer.handle().underlying());

                  const auto count = std::strlen( value) + 1;
                  const auto space = buffer.payload.data.size();

                  if( count > space)
                  {
                     // We need to report this
                     return CASUAL_STRING_OUT_OF_BOUNDS;
                  }
               }
               catch( ...)
               {
                  return error();
               }

               return CASUAL_STRING_SUCCESS;
            }

         } //

      } // string

   } // buffer

} // casual

const char* casual_string_description( const int code)
{
   switch( code)
   {
      case CASUAL_STRING_SUCCESS:
         return "Success";
      case CASUAL_STRING_INVALID_HANDLE:
         return "Invalid handle";
      case CASUAL_STRING_INVALID_ARGUMENT:
         return "Invalid argument";
      case CASUAL_STRING_OUT_OF_MEMORY:
         return "Out of memory";
      case CASUAL_STRING_OUT_OF_BOUNDS:
         return "Out of bounds";
      case CASUAL_STRING_INTERNAL_FAILURE:
         return "Internal failure";
      default:
         return "Unknown code";
   }

}

int casual_string_explore_buffer( const char* const handle, long* const size, long* const used)
{
   return casual::buffer::string::explore( handle, size, used);
}

int casual_string_set( char** const handle, const char* const value)
{
   return casual::buffer::string::set( handle, value);
}

int casual_string_get( const char* handle, const char** value)
{
   return casual::buffer::string::get( handle, *value);
}

