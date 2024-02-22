//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "casual/buffer/order.h"

#include "common/buffer/pool.h"
#include "common/buffer/type.h"
#include "common/exception/capture.h"
#include "common/network/byteorder.h"
#include "casual/platform.h"
#include "common/log.h"
#include "common/algorithm.h"
#include "common/execute.h"

#include "common/code/xatmi.h"

#include <cstring>
#include <utility>

namespace casual
{
   namespace buffer
   {
      namespace order
      {

         using size_type = platform::size::type;
         using const_data_type = platform::binary::type::const_pointer;
         using data_type = platform::binary::type::pointer;

         namespace local
         {
            namespace
            {

               struct Buffer : public common::buffer::Buffer
               {
                  using common::buffer::Buffer::Buffer;
                  using size_type = platform::binary::size::type;

                  void shrink()
                  {
                     payload.data.shrink_to_fit();

                     if( ! payload.data.data())
                     {
                        payload.data.reserve( 1);
                     }
                  }

                  size_type capacity() const noexcept
                  {
                     return payload.data.capacity();
                  }

                  void capacity( const decltype(payload.data.capacity()) value)
                  {
                     payload.data.reserve( value);
                  }

                  size_type utilized() const noexcept
                  {
                     return payload.data.size();
                  }

                  void utilized( const decltype(payload.data.size()) value)
                  {
                     payload.data.resize( value);
                  }

                  size_type consumed() const noexcept
                  {
                     return selector;
                  }

                  void consumed( const decltype(payload.data.size()) value) noexcept
                  {
                     selector = value;
                  }

                  auto handle() const noexcept { return common::buffer::handle::type{ payload.data.data()};}
                  auto handle() noexcept { return common::buffer::handle::mutate::type{ payload.data.data()};}

                  //! Implement Buffer::transport
                  size_type transport( const platform::binary::size::type user_size) const
                  {
                     // Just ignore user-size all together
                     return utilized();
                  }

                  //! Implement Buffer::reserved
                  auto reserved() const
                  {
                     return capacity();
                  }

               private:
                  decltype( payload.data.size()) selector = 0;
               };

               using allocator_base = common::buffer::pool::implementation::Default< Buffer>;
               struct Allocator : allocator_base
               {
                  static constexpr auto types() 
                  {
                     constexpr std::string_view key = CASUAL_ORDER "/";
                     return common::array::make( key);
                  };

                  common::buffer::handle::mutate::type allocate( std::string_view type, platform::binary::size::type size)
                  {
                     auto& buffer = allocator_base::emplace_back( type, 0);

                     buffer.capacity( size);

                     return buffer.handle();
                  }


                  common::buffer::handle::mutate::type reallocate( common::buffer::handle::type handle, platform::binary::size::type size)
                  {
                     auto& buffer = allocator_base::get( handle);

                     if( size < buffer.utilized())
                     {
                        // Allow user to reduce allocation
                        buffer.shrink();
                     }
                     else
                     {
                        buffer.capacity( size);
                     }

                     return buffer.handle();
                  }

               };

            } //
         } // local

         namespace
         {
            using pool_type = common::buffer::pool::Registration< local::Allocator>;

            namespace error
            {
               int handle()
               {
                  try
                  {
                     throw;
                  }
                  catch( const std::bad_alloc&)
                  {
                     return CASUAL_ORDER_OUT_OF_MEMORY;
                  }
                  catch( const std::out_of_range&)
                  {
                     return CASUAL_ORDER_OUT_OF_BOUNDS;
                  }
                  catch( ...)
                  {
                     const auto error = common::exception::capture();

                     if( error.code() == common::code::xatmi::argument)
                        return CASUAL_ORDER_INVALID_HANDLE; 

                     common::log::line( common::log::category::error, error);

                     return CASUAL_ORDER_INTERNAL_FAILURE;
                  }
               }
            } // error


            namespace explore
            {

               int buffer( const char* const handle, size_type* const reserved, size_type* const utilized, size_type* const consumed) noexcept
               {
                  try
                  {
                     const auto& buffer = pool_type::pool().get( common::buffer::handle::type{ handle});

                     if( reserved) *reserved = buffer.capacity();
                     if( utilized) *utilized = buffer.utilized();
                     if( consumed) *consumed = buffer.consumed();
                  }
                  catch( ...)
                  {
                     return error::handle();
                  }

                  return CASUAL_ORDER_SUCCESS;
               }

            } // explore


            namespace add
            {

               int reset( const char* const handle) noexcept
               {
                  try
                  {
                     pool_type::pool().get( common::buffer::handle::type{ handle}).utilized( 0);
                  }
                  catch( ...)
                  {
                     return error::handle();
                  }

                  return CASUAL_ORDER_SUCCESS;
               }

               template<typename M, typename T>
               void append( M& memory, const T value)
               {
                  const auto encoded = common::network::byteorder::encode( value);
                  const auto data = reinterpret_cast<const_data_type>( &encoded);
                  const constexpr auto size = sizeof( encoded);

                  memory.insert( memory.end(), data, data + size);
               }

               template<typename M>
               void append( M& memory, const_data_type data)
               {
                  const auto size = std::strlen( data) + 1;

                  memory.insert( memory.end(), data, data + size);
               }


               template<typename M>
               void append( M& memory, const_data_type data, const size_type size)
               {
                  //
                  // Make sure to reset the size in case of exception since
                  // this is not an atomic operation
                  //

                  //
                  // capture current size
                  //
                  const auto used = memory.size();

                  //
                  // create potential rollback
                  //
                  const auto reset = common::execute::scope
                  ( [&](){ if( std::uncaught_exceptions()) memory.resize( used);});

                  //
                  // append first size chunk
                  //
                  append( memory, size);

                  //
                  // append other data chunk
                  //
                  memory.insert( memory.end(), data, data + size);
               }

               template<typename... A>
               int data( char** handle, A&&... arguments) noexcept
               {
                  try
                  {
                     auto& buffer = pool_type::pool().get( common::buffer::handle::type{ *handle});

                     // Make sure to update the handle regardless
                     const auto synchronize = common::execute::scope( [ handle, &buffer]() 
                     { 
                        *handle = buffer.handle().underlying();
                     });

                     // Append the data
                     append( buffer.payload.data, std::forward<A>( arguments)...);

                  }
                  catch( ...)
                  {
                     return error::handle();
                  }

                  return CASUAL_ORDER_SUCCESS;
               }

            } // add

            namespace get
            {
               int reset( const char* const handle) noexcept
               {
                  try
                  {
                     pool_type::pool().get( common::buffer::handle::type{ handle}).consumed( 0);
                  }
                  catch( ...)
                  {
                     return error::handle();
                  }

                  return CASUAL_ORDER_SUCCESS;
               }


               template<typename T>
               size_type select( const_data_type where, T& value) noexcept
               {
                  const auto encoded = *reinterpret_cast< const common::network::byteorder::type<T>*>( where);

                  value = common::network::byteorder::decode<T>( encoded);

                  return sizeof( encoded);
               }

               size_type select( const_data_type where, const_data_type& value) noexcept
               {
                  value = where;

                  return std::strlen( value) + 1;
               }

               size_type select( const_data_type where, const_data_type& data, size_type& size) noexcept
               {
                  const auto read = select( where, size);
                  data = where + read;
                  return read + size;
               }

               template<typename... A>
               int data( const char* const handle, A&&... arguments) noexcept
               {
                  try
                  {
                     auto& buffer = pool_type::pool().get( common::buffer::handle::type{ handle});

                     // See how much have been read so far
                     const auto read = buffer.consumed();

                     // Read the data and let us not how much
                     const auto size = select( buffer.handle().underlying() + read, std::forward<A>( arguments)...);

                     // Calculate the new cursor
                     const auto consumed = read + size;

                     if( consumed > buffer.utilized())
                        return CASUAL_ORDER_OUT_OF_BOUNDS; // We need to report this

                     // Update the cursor
                     buffer.consumed( consumed);
                  }
                  catch( ...)
                  {
                     return error::handle();
                  }

                  return CASUAL_ORDER_SUCCESS;
               }


            } // get

         } //

      } // order

   } // Buffer



} // casual



const char* casual_order_description( const int code)
{
   switch( code)
   {
      case CASUAL_ORDER_SUCCESS:
         return "Success";
      case CASUAL_ORDER_INVALID_HANDLE:
         return "Invalid handle";
      case CASUAL_ORDER_INVALID_ARGUMENT:
         return "Invalid argument";
      case CASUAL_ORDER_OUT_OF_MEMORY:
         return "Out of memory";
      case CASUAL_ORDER_OUT_OF_BOUNDS:
         return "Out of bounds";
      case CASUAL_ORDER_INTERNAL_FAILURE:
         return "Internal failure";
      default:
         return "Unknown code";
   }
}

int casual_order_explore_buffer( const char* const buffer, long* const reserved, long* const utilized, long* const consumed)
{
   return casual::buffer::order::explore::buffer( buffer, reserved, utilized, consumed);
}

int casual_order_add_prepare( const char* const buffer)
{
   return casual::buffer::order::add::reset( buffer);
}

int casual_order_add_bool( char** buffer, const bool value)
{
   return casual::buffer::order::add::data( buffer, value);
}

int casual_order_add_char( char** buffer, const char value)
{
   return casual::buffer::order::add::data( buffer, value);
}

int casual_order_add_short( char** buffer, const short value)
{
   return casual::buffer::order::add::data( buffer, value);
}

int casual_order_add_long( char** buffer, const long value)
{
   return casual::buffer::order::add::data( buffer, value);
}

int casual_order_add_float( char** buffer, const float value)
{
   return casual::buffer::order::add::data( buffer, value);
}

int casual_order_add_double( char** buffer, const double value)
{
   return casual::buffer::order::add::data( buffer, value);
}

int casual_order_add_string( char** buffer, const char* const value)
{
   return casual::buffer::order::add::data( buffer, value);
}

int casual_order_add_binary( char** buffer, const char* const data, const long size)
{
   if( size < 0)
   {
      return CASUAL_ORDER_INVALID_ARGUMENT;
   }

   return casual::buffer::order::add::data( buffer, data, size);
}

int casual_order_get_prepare( const char* const buffer)
{
   return casual::buffer::order::get::reset( buffer);
}

int casual_order_get_bool( const char* const buffer, bool* const value)
{
   return casual::buffer::order::get::data( buffer, *value);
}

int casual_order_get_char( const char* const buffer, char* const value)
{
   return casual::buffer::order::get::data( buffer, *value);
}

int casual_order_get_short( const char* const buffer, short* const value)
{
   return casual::buffer::order::get::data( buffer, *value);
}

int casual_order_get_long( const char* const buffer, long* const value)
{
   return casual::buffer::order::get::data( buffer, *value);
}

int casual_order_get_float( const char* const buffer, float* const value)
{
   return casual::buffer::order::get::data( buffer, *value);
}

int casual_order_get_double( const char* const buffer, double* const value)
{
   return casual::buffer::order::get::data( buffer, *value);
}

int casual_order_get_string( const char* const buffer, const char** value)
{
   return casual::buffer::order::get::data( buffer, *value);
}

int casual_order_get_binary( const char* const buffer, const char** data, long* const size)
{
   return casual::buffer::order::get::data( buffer, *data, *size);
}
