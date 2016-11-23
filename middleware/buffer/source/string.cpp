//!
//! string.cpp
//!
//! Created on: Dec 26, 2013
//!     Author: Lazan
//!

#include "buffer/string.h"

#include "common/buffer/pool.h"
#include "common/buffer/type.h"

#include "common/log.h"
#include "common/memory.h"

#include "common/internal/trace.h"


#include <cstring>

namespace casual
{
   namespace buffer
   {
      namespace string
      {

         namespace
         {

/*
            struct trace : common::trace::basic::Scope
            {
               template<decltype(sizeof("")) size>
               explicit trace( const char (&information)[size]) : Scope( information, common::log::internal::buffer) {}
            };
*/

            typedef common::platform::binary_type::size_type size_type;
            typedef common::platform::raw_buffer_type data_type;


            namespace local
            {
               size_type validate( const common::buffer::Payload& payload)
               {
                  const auto size = payload.memory.size();
                  const auto used = std::strlen( payload.memory.data()) + 1;

                  //
                  // We do need to check that it is a real null-terminated
                  // string within allocated area
                  //

                  if( used > size)
                  {
                     throw common::exception::xatmi::invalid::Argument{ "string is longer than allocated size"};
                  }

                  return used;

               }

            }


            struct Buffer : common::buffer::Buffer
            {
               using common::buffer::Buffer::Buffer;

               //!
               //! Implement Buffer::transport
               //!
               size_type transport( const size_type user_size) const
               {
                  //
                  // Just ignore user-size all together
                  //
                  // ... but we do need to validate it
                  //
                  return local::validate( payload);
               }

            };



            class Allocator : public common::buffer::pool::basic_pool<Buffer>
            {
            public:

               using types_type = common::buffer::pool::default_pool::types_type;

               static const types_type& types()
               {
                  // The types this pool can manage
                  static const types_type result{ common::buffer::type::combine( CASUAL_STRING)};
                  return result;
               }

               common::platform::raw_buffer_type allocate( const std::string& type, const common::platform::binary_size_type size)
               {
                  m_pool.emplace_back( type, size ? size : 1);

                  m_pool.back().payload.memory.front() = '\0';

                  return m_pool.back().payload.memory.data();
               }

               common::platform::raw_buffer_type reallocate( const common::platform::const_raw_buffer_type handle, const common::platform::binary_size_type size)
               {
                  const auto result = find( handle);

                  result->payload.memory.resize( size ? size : 1);

                  result->payload.memory.back() = '\0';

                  // Allow user to reduce allocation
                  result->payload.memory.shrink_to_fit();

                  return result->payload.memory.data();
               }

               common::platform::raw_buffer_type insert( common::buffer::Payload payload)
               {
                  //
                  // Validate it before we move it
                  //
                  local::validate( payload);

                  m_pool.emplace_back( std::move( payload));

                  return m_pool.back().payload.memory.data();
               }

            };

         } // <unnamed>

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
               catch( const std::bad_alloc&)
               {
                  return CASUAL_STRING_OUT_OF_MEMORY;
               }
               catch( const common::exception::xatmi::invalid::Argument&)
               {
                  return CASUAL_STRING_INVALID_HANDLE;
               }
               catch( ...)
               {
                  common::error::handler();
                  return CASUAL_STRING_INTERNAL_FAILURE;
               }
            }


            int explore( const char* const handle, long* const size, long* const used)
            {
               //const trace trace( "string::explore");

               try
               {
                  const auto& buffer = pool_type::pool.get( handle);

                  if( size) *size = buffer.payload.memory.size();
                  if( used) *used = std::strlen( buffer.payload.memory.data()) + 1;
               }
               catch( ...)
               {
                  return error();
               }

               return CASUAL_STRING_SUCCESS;
            }


            int set( char** const handle, const char* const value)
            {
               //const trace trace( "string::set");

               try
               {
                  auto& buffer = pool_type::pool.get( *handle);

                  const auto count = std::strlen( value) + 1;

                  if( count > buffer.payload.memory.size())
                  {
                     buffer.payload.memory.resize( count);
                     *handle = buffer.payload.memory.data();
                  }

                  casual::common::memory::copy(
                     casual::common::range::make( value, count),
                     casual::common::range::make( buffer.payload.memory));

               }
               catch( ...)
               {
                  return error();
               }

               return CASUAL_STRING_SUCCESS;

            }

            int get( const char* const handle, const char** value)
            {
               //const trace trace( "string::get");

               try
               {
                  const auto& buffer = pool_type::pool.get( handle);

                  const auto used = std::strlen( buffer.payload.memory.data()) + 1;
                  const auto size = buffer.payload.memory.size();

                  if( used > size)
                  {
                     //
                     // We need to report this
                     //
                     buffer.payload.memory.at( used);
                  }

                  if( value) *value = buffer.payload.memory.data();

               }
               catch( ...)
               {
                  return error();
               }

               return CASUAL_STRING_SUCCESS;

            }

         } // <unnamed>

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
   return casual::buffer::string::get( handle, value);
}

