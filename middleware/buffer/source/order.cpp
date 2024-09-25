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

#include <utility>
#include <memory>

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

                  void capacity( const auto value)
                  {
                     payload.data.reserve( value);
                  }

                  size_type utilized() const noexcept
                  {
                     return payload.data.size();
                  }

                  void utilized( const auto value)
                  {
                     payload.data.resize( value);
                  }

                  size_type consumed() const noexcept
                  {
                     return selector;
                  }

                  void consumed( const auto value) noexcept
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
                  decltype( payload.data.size()) selector{};
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

               auto append( auto& memory, const auto value)
               {
                  const auto encoded = common::network::byteorder::encode( value);
                  common::algorithm::container::append( std::as_bytes( std::span{ &encoded, 1}), memory);
               }

               auto append( auto& memory, const char* const value)
               {
                  common::algorithm::container::append( common::binary::span::make( value, std::strlen( value) + 1), memory);
               }

               auto append( auto& memory, const char* const data, const size_type size)
               {
                  const auto used = memory.size();

                  try
                  {
                     // append first size chunk
                     append( memory, size);
                     // append other data chunk
                     common::algorithm::container::append( common::binary::span::make( data, size), memory);
                  }
                  catch(const std::exception& e)
                  {
                     // make sure to reset the size in case of exception
                     memory.resize( used);
                     throw;
                  }
               }

               template<typename... A>
               int data( char** handle, A&&... arguments) noexcept
               {
                  try
                  {
                     auto& buffer = pool_type::pool().get( common::buffer::handle::type{ *handle});

                     // make sure to update the handle regardless
                     const auto synchronize = common::execute::scope( [ handle, &buffer]() 
                     { 
                        *handle = buffer.handle().raw();
                     });

                     // append the data
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
                  //value = common::network::byteorder::decode< T>( *std::start_lifetime_as< common::network::byteorder::type< T>>( where));
                  value = common::network::byteorder::decode< T>( *reinterpret_cast< const common::network::byteorder::type< T>*>( where));
                  return common::network::byteorder::bytes< T>();
               }

               size_type select( const_data_type where, const char*& value) noexcept
               {
                  value = reinterpret_cast< const char*>( where);
                  return std::strlen( value) + 1;
               }

               size_type select( const_data_type where, const char*& data, size_type& size) noexcept
               {
                  // select first size chunk
                  const auto read = select( where, size);
                  data = reinterpret_cast< const char*>( where + read);
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
                     {
                        // We need to report this
                        return CASUAL_ORDER_OUT_OF_BOUNDS;
                     }

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

int casual_order_explore_buffer( const char* const handle, long* const reserved, long* const utilized, long* const consumed)
{
   return casual::buffer::order::explore::buffer( handle, reserved, utilized, consumed);
}

int casual_order_add_prepare( const char* const handle)
{
   return casual::buffer::order::add::reset( handle);
}

int casual_order_add_bool( char** handle, const bool value)
{
   return casual::buffer::order::add::data( handle, value);
}

int casual_order_add_char( char** handle, const char value)
{
   return casual::buffer::order::add::data( handle, value);
}

int casual_order_add_short( char** handle, const short value)
{
   return casual::buffer::order::add::data( handle, value);
}

int casual_order_add_long( char** handle, const long value)
{
   return casual::buffer::order::add::data( handle, value);
}

int casual_order_add_float( char** handle, const float value)
{
   return casual::buffer::order::add::data( handle, value);
}

int casual_order_add_double( char** handle, const double value)
{
   return casual::buffer::order::add::data( handle, value);
}

int casual_order_add_string( char** handle, const char* const value)
{
   return casual::buffer::order::add::data( handle, value);
}

int casual_order_add_binary( char** handle, const char* const data, const long size)
{
   if( size < 0)
      return CASUAL_ORDER_INVALID_ARGUMENT;

   return casual::buffer::order::add::data( handle, data, size);
}

int casual_order_get_prepare( const char* const handle)
{
   return casual::buffer::order::get::reset( handle);
}

int casual_order_get_bool( const char* const handle, bool* const value)
{
   return casual::buffer::order::get::data( handle, *value);
}

int casual_order_get_char( const char* const handle, char* const value)
{
   return casual::buffer::order::get::data( handle, *value);
}

int casual_order_get_short( const char* const handle, short* const value)
{
   return casual::buffer::order::get::data( handle, *value);
}

int casual_order_get_long( const char* const handle, long* const value)
{
   return casual::buffer::order::get::data( handle, *value);
}

int casual_order_get_float( const char* const handle, float* const value)
{
   return casual::buffer::order::get::data( handle, *value);
}

int casual_order_get_double( const char* const handle, double* const value)
{
   return casual::buffer::order::get::data( handle, *value);
}

int casual_order_get_string( const char* const handle, const char** value)
{
   return casual::buffer::order::get::data( handle, *value);
}

int casual_order_get_binary( const char* const handle, const char** data, long* const size)
{
   return casual::buffer::order::get::data( handle, *data, *size);
}
