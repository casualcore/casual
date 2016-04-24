//
// casual_order_buffer.cpp
//
//  Created on: 20 okt 2013
//      Author: Kristone
//

#include "buffer/order.h"

#include "common/buffer/pool.h"
#include "common/buffer/type.h"
#include "common/exception.h"
#include "common/network/byteorder.h"
#include "common/platform.h"
#include "common/log.h"
#include "common/internal/trace.h"

#include <cstring>
#include <utility>

namespace casual
{
   namespace buffer
   {
      namespace order
      {

         /*
          * This implementation contain a C-interface that offers functionality
          * for serializing data in an XATMI-environment
          *
          * To use it you need to get the data in the same order as you add it
          * since no validation of types or fields what so ever occurs. The
          * only validation that occurs is to check whether you try to consume
          * (get) beyond what's inserted
          *
          * The implementation is quite cumbersome since the user-handle is the
          * actual underlying buffer which is a std::vector<char>::data() so we
          * cannot just use append data which might imply reallocation since
          * this is a "semi-stateless" interface
          *
          * The main idea is to keep the buffer "ready to go" without the need
          * for extra marshalling when transported and thus data is stored in
          * network byteorder etc from start
          *
          * The inserted-parameter is offset just past last write
          * The selected-parameter is offset just past last parse
          *
          * String is stored with null-termination
          *
          * Binary is stored with a size (network-long) and then it's data
          *
          */

         namespace
         {

            typedef common::platform::binary_type::size_type size_type;
            typedef common::platform::binary_type::const_pointer const_data_type;
            typedef common::platform::binary_type::pointer data_type;


            struct Buffer : public common::buffer::Buffer
            {
               using common::buffer::Buffer::Buffer;

               common::platform::binary_type::size_type selector = 0;

               size_type capacity() const noexcept
               {
                  return payload.memory.capacity();
               }

               void capacity( const size_type value)
               {
                  payload.memory.reserve( value);
               }

               size_type utilized() const noexcept
               {
                  return payload.memory.size();
               }

               void utilized( const size_type value)
               {
                  payload.memory.resize( value);
               }

               size_type consumed() const noexcept
               {
                  return selector;
               }

               void consumed( const size_type value) noexcept
               {
                  selector = value;
               }

               const_data_type handle() const noexcept
               {
                  return payload.memory.data();
               }

               data_type handle() noexcept
               {
                  return payload.memory.data();
               }


               size_type transport( const size_type user_size) const
               {
                  //
                  // We could ignore user-size all together, but something is
                  // wrong if user supplies a greater size than allocated
                  //
                  //if( user_size > utilized())
                  //{
                  //   throw common::exception::xatmi::invalid::Argument{ "user supplied size is larger than allocated size"};
                  //}

                  return utilized();
               }

            };


            class Allocator : public common::buffer::pool::basic_pool<Buffer>
            {
            public:

               using types_type = common::buffer::pool::default_pool::types_type;

               static const types_type& types()
               {
                  // The types this pool can manage
                  static const types_type result{{ CASUAL_ORDER, "" }};
                  return result;
               }

               common::platform::raw_buffer_type allocate( const common::buffer::Type& type, const common::platform::binary_size_type size)
               {
                  m_pool.emplace_back( type, 0);
                  // GCC returns null for std::vector::data with size zero
                  m_pool.back().payload.memory.reserve( size ? size : 1);
                  return m_pool.back().payload.memory.data();
               }


               common::platform::raw_buffer_type reallocate( const common::platform::const_raw_buffer_type handle, const common::platform::binary_size_type size)
               {
                  const auto result = find( handle);
                  // Allow user to reduce allocation
                  if( size < result->payload.memory.capacity()) result->payload.memory.shrink_to_fit();
                  // GCC returns null for std::vector::data with size zero
                  result->payload.memory.reserve( size ? size : 1);
                  return result->payload.memory.data();
               }

               common::platform::raw_buffer_type insert( common::buffer::Payload payload)
               {
                  m_pool.emplace_back( std::move( payload));
                  return m_pool.back().payload.memory.data();
               }

            };

         } // <unnamed>

      } // order
   } // buffer

   //
   // Register and define the type that can be used to get the custom pool
   //
   template class common::buffer::pool::Registration< buffer::order::Allocator>;

   namespace buffer
   {
      namespace order
      {
         using pool_type = common::buffer::pool::Registration< Allocator>;

         namespace
         {
/*
            struct trace : common::trace::basic::Scope
            {
               template<decltype(sizeof("")) size>
               explicit trace( const char (&information)[size]) : Scope( information, common::log::internal::buffer) {}
            };
*/

            int explore( const char* const handle, long* const reserved, long* const utilized, long* const consumed)
            {
               //const trace trace( "order::explore");

               try
               {
                  const auto& buffer = pool_type::pool.get( handle);

                  if( reserved) *reserved = static_cast<long>(buffer.capacity());
                  if( utilized) *utilized = static_cast<long>(buffer.utilized());
                  if( consumed) *consumed = static_cast<long>(buffer.consumed());
               }
               catch( const common::exception::xatmi::invalid::Argument&)
               {
                  return CASUAL_ORDER_INVALID_HANDLE;
               }
               catch( ...)
               {
                  common::error::handler();
                  return CASUAL_ORDER_INTERNAL_FAILURE;
               }

               return CASUAL_ORDER_SUCCESS;
            }

            namespace add
            {

               int reset( const char* const handle)
               {
                  //const trace trace( "order::add::reset");

                  try
                  {
                     pool_type::pool.get( handle).utilized( 0);
                  }
                  catch( const common::exception::xatmi::invalid::Argument&)
                  {
                     return CASUAL_ORDER_INVALID_HANDLE;
                  }
                  catch( ...)
                  {
                     common::error::handler();
                     return CASUAL_ORDER_INTERNAL_FAILURE;
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
               void append( M& memory, const_data_type data, const long size)
               {
                  append( memory, size);

                  memory.insert( memory.end(), data, data + size);
               }


               //
               // TODO: std::function and/or lambdas are slow
               //
               struct exit
               {
                  std::function< void()> executor;
                  exit( std::function< void()> function) : executor( std::move( function)) {}
                  ~exit() { executor();}
               };

               template<typename... A>
               int data( char** handle, A&&... arguments)
               {
                  //const trace trace( "order::add::data");

                  try
                  {
                     auto& buffer = pool_type::pool.get( *handle);


                     //
                     // Make sure to reset the size in case of exception
                     //
                     const auto used = buffer.utilized();
                     const exit reset
                     { [&](){ if( std::uncaught_exception()) buffer.utilized( used);}};


                     //
                     // Make sure to update the handle regardless
                     //
                     const exit synchronize
                     { [&]() { *handle = buffer.handle();}};


                     //
                     // Append the data
                     //
                     append( buffer.payload.memory, std::forward<A>( arguments)...);

                  }
                  catch( const std::bad_alloc&)
                  {
                     return CASUAL_ORDER_OUT_OF_MEMORY;
                  }
                  catch( const common::exception::xatmi::invalid::Argument&)
                  {
                     return CASUAL_ORDER_INVALID_HANDLE;
                  }
                  catch( ...)
                  {
                     common::error::handler();
                     return CASUAL_ORDER_INTERNAL_FAILURE;
                  }

                  return CASUAL_ORDER_SUCCESS;
               }

            } // add

            namespace get
            {
               int reset( const char* const handle)
               {
                  //const trace trace( "order::get::reset");

                  try
                  {
                     pool_type::pool.get( handle).consumed( 0);
                  }
                  catch( const common::exception::xatmi::invalid::Argument&)
                  {
                     return CASUAL_ORDER_INVALID_HANDLE;
                  }
                  catch( ...)
                  {
                     common::error::handler();
                     return CASUAL_ORDER_INTERNAL_FAILURE;
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

               size_type select( const_data_type where, const_data_type& data, long& size) noexcept
               {
                  const auto read = select( where, size);
                  data = where + read;
                  return read + size;
               }

               template<typename... A>
               int data( const char* const handle, A&&... arguments)
               {
                  //const trace trace( "order::add::data");

                  try
                  {
                     auto& buffer = pool_type::pool.get( handle);

                     //
                     // See how much have been read so far
                     //
                     const auto read = buffer.consumed();

                     //
                     // Read the data and let us not how much
                     //
                     const auto size = select( buffer.handle() + read, std::forward<A>( arguments)...);

                     //
                     // Calculate the new cursor
                     //
                     const auto consumed = read + size;

                     if( consumed > buffer.utilized())
                     {
                        //
                        // We need to report this
                        //
                        return CASUAL_ORDER_OUT_OF_BOUNDS;
                     }

                     //
                     // Update the cursor
                     //
                     buffer.consumed( consumed);

                  }
                  catch( const common::exception::xatmi::invalid::Argument&)
                  {
                     return CASUAL_ORDER_INVALID_HANDLE;
                  }
                  catch( ...)
                  {
                     common::error::handler();
                     return CASUAL_ORDER_INTERNAL_FAILURE;
                  }

                  return CASUAL_ORDER_SUCCESS;
               }


            } // get


         } // <unnamed>

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
      case CASUAL_ORDER_OUT_OF_BOUNDS:
         return "Out of bounds";
      case CASUAL_ORDER_OUT_OF_MEMORY:
         return "Out of memory";
      case CASUAL_ORDER_INTERNAL_FAILURE:
         return "Internal failure";
      default:
         return "Unknown code";
   }
}

int casual_order_explore_buffer( const char* const buffer, long* const reserved, long* const utilized, long* const consumed)
{
   return casual::buffer::order::explore( buffer, reserved, utilized, consumed);
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

