//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "casual/buffer/octet.h"

#include "common/buffer/pool.h"
#include "common/memory.h"
#include "casual/platform.h"
#include "common/exception/capture.h"

namespace casual
{
   namespace buffer
   {
      namespace octet
      {
         namespace
         {

            using size_type = platform::size::type;
            using const_data_type = platform::binary::type::const_pointer;
            using data_type = platform::binary::type::pointer;


            struct Buffer : common::buffer::Buffer
            {
               using common::buffer::Buffer::Buffer;

               //! Implement Buffer::transport
               size_type transport( platform::size::type user_size) const
               {

                  // Just ignore user-size all together
                  return payload.data.size();
               }
            };

            using allocator_base = common::buffer::pool::implementation::Default< Buffer>;
            struct Allocator : allocator_base
            {

               static constexpr auto types() 
               {
                  return common::array::force::make< std::string_view>( 
                     CASUAL_OCTET "/",
                     CASUAL_OCTET "/" CASUAL_OCTET_XML,
                     CASUAL_OCTET "/" CASUAL_OCTET_JSON,
                     CASUAL_OCTET "/" CASUAL_OCTET_YAML
                  );
               };

               common::buffer::handle::mutate::type allocate( std::string_view type, platform::binary::size::type size)
               {
                  return allocator_base::emplace_back( type, size).handle();
               }


               common::buffer::handle::mutate::type reallocate( common::buffer::handle::type handle, platform::binary::size::type size)
               {
                  auto& buffer = allocator_base::get( handle);

                  buffer.payload.data.resize( size);

                  // Allow user to reduce allocation
                  buffer.payload.data.shrink_to_fit();

                  if( ! buffer.payload.data.data())
                  {
                     buffer.payload.data.reserve( 1);
                  }

                  return buffer.payload.handle();
               }
            };

            using pool_type = common::buffer::pool::Registration< Allocator>;

            int error() noexcept
            {
               try
               {
                  throw;
               }
               catch( const std::bad_alloc&)
               {
                  return CASUAL_OCTET_OUT_OF_MEMORY;
               }
               catch( ...)
               {
                  const auto error = common::exception::capture();

                  if( error.code() == common::code::xatmi::argument)
                     return CASUAL_OCTET_INVALID_HANDLE; 

                  common::log::line( common::log::category::error, error);

                  return CASUAL_OCTET_INTERNAL_FAILURE;
               }
            }


            int explore( const char* const handle, const char** name, size_type* const size) noexcept
            {
               try
               {
                  const auto& buffer = pool_type::pool().get( common::buffer::handle::type{ handle});


                  if( name)
                     *name = std::get< 1>( common::algorithm::split( buffer.payload.type, '/')).data();

                  if( size)
                     *size = buffer.payload.data.size();
               }
               catch( ...)
               {
                  return error();
               }

               return CASUAL_OCTET_SUCCESS;
            }


            int set( char** const handle, const char* const data, const long size) noexcept
            {
               try
               {
                  auto& buffer = pool_type::pool().get( common::buffer::handle::type{ *handle});

                  casual::common::algorithm::copy( casual::common::binary::span::make( data, size), buffer.payload.data);

                  *handle = buffer.payload.handle().raw();
               }
               catch( ...)
               {
                  return error();
               }

               return CASUAL_OCTET_SUCCESS;
            }

            int get( const char* const handle, const char*& data, size_type& size)
            {
               try
               {
                  const auto& buffer = pool_type::pool().get( common::buffer::handle::type{ handle});

                  data = reinterpret_cast< const char*>( buffer.handle().underlying());
                  size = buffer.payload.data.size();
               }
               catch( ...)
               {
                  return error();
               }
               
               return CASUAL_OCTET_SUCCESS;
            }
         } //

      } // octet

   } // buffer

} // casual


const char* casual_octet_description( const int code)
{
   switch( code)
   {
      case CASUAL_OCTET_SUCCESS:
         return "Success";
      case CASUAL_OCTET_INVALID_HANDLE:
         return "Invalid handle";
      case CASUAL_OCTET_INVALID_ARGUMENT:
         return "Invalid argument";
      case CASUAL_OCTET_OUT_OF_MEMORY:
         return "Out of memory";
      case CASUAL_OCTET_INTERNAL_FAILURE:
         return "Internal failure";
      default:
         return "Unknown code";
   }

}

int casual_octet_explore_buffer( const char* const handle, const char** name, long* const size)
{
   return casual::buffer::octet::explore( handle, name, size);
}


int casual_octet_set( char** handle, const char* const data, const long size)
{
   if( size < 0)
   {
      return CASUAL_OCTET_INVALID_ARGUMENT;
   }

   return casual::buffer::octet::set( handle, data, size);
}

int casual_octet_get( const char* const handle, const char** data, long* const size)
{
   return casual::buffer::octet::get( handle, *data, *size);
}
