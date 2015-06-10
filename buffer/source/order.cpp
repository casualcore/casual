//
// casual_order_buffer.cpp
//
//  Created on: 20 okt 2013
//      Author: Kristone
//

#include "buffer/order.h"

#include "common/buffer/pool.h"
#include "common/buffer/type.h"
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

            class Buffer : public common::buffer::Buffer
            {
            public:

               using common::buffer::Buffer::Buffer;

               typedef common::platform::binary_type::size_type size_type;
               typedef common::platform::binary_type::const_pointer const_data_type;


               size_type transport( size_type user_size) const
               {
                  //
                  // We could ignore user-size all together, but something is
                  // wrong if user supplies a greater size than allocated
                  //
                  if( user_size > reserved())
                  {
                     throw common::exception::xatmi::InvalidArguments{ "user supplied size is larger than allocated size"};
                  }

                  return utilized();
               }

               size_type reserved() const
               {
                  return payload.memory.capacity();
               }

               size_type utilized() const
               {
                  return payload.memory.size();
               }

               size_type consumed() const
               {
                  return m_selector;
               }

            public:

               void clear()
               {
                  payload.memory.clear();
                  m_selector = 0;
               }

               void reset()
               {
                  m_selector = 0;
               }

               template<typename T>
               bool append( const T value)
               {
                  const auto encoded = common::network::byteorder::encode( value);
                  const constexpr auto count = sizeof( encoded);

                  if( (reserved() - utilized()) < count)
                  {
                     return false;
                  }

                  payload.memory.insert(
                     payload.memory.end(),
                     reinterpret_cast<const_data_type>( &encoded),
                     reinterpret_cast<const_data_type>( &encoded) + count);

                  return true;
               }

               bool append( const char* const value)
               {
                  const auto count = std::strlen( value) + 1;

                  if( (reserved() - utilized()) < count)
                  {
                     return false;
                  }

                  payload.memory.insert( payload.memory.end(), value, value + count);

                  return true;
               }


               bool append( const_data_type data, const long size)
               {
                  const auto total = common::network::byteorder::bytes<long>() + size;

                  if( (reserved() - utilized()) < total)
                  {
                     return false;
                  }

                  append( size);

                  payload.memory.insert( payload.memory.end(), data, data + size);

                  return true;
               }


               template<typename T>
               bool select( T& value)
               {
                  const auto where = payload.memory.data() + m_selector;

                  const auto encoded = *reinterpret_cast< const common::network::byteorder::type<T>*>( where);

                  const constexpr auto count = sizeof( encoded);

                  if( (utilized() - consumed()) < count)
                  {
                     return false;
                  }

                  value = common::network::byteorder::decode<T>( encoded);

                  m_selector += count;

                  return true;
               }

               bool select( const char*& value)
               {
                  const auto where = payload.memory.data() + m_selector;

                  const auto count = std::strlen( where) + 1;

                  if( (utilized() - consumed()) < count)
                  {
                     return false;
                  }

                  value = where;

                  m_selector += count;

                  return true;
               }

               bool select( const_data_type& data, long& size)
               {
                  if( select( size))
                  {
                     if( (consumed() + size) > utilized())
                     {
                        //
                        // Perhaps unnecessary to reverse
                        //
                        m_selector -= common::network::byteorder::bytes<long>();

                        return false;
                     }

                  }
                  else
                  {
                     return false;
                  }

                  data = payload.memory.data() + m_selector;

                  m_selector += size;

                  return true;

               }

               static bool copy( Buffer& target, const Buffer& source)
               {
                  if( target.reserved() < source.utilized())
                  {
                     return false;
                  }

                  target.payload.memory = source.payload.memory;

                  //
                  // TODO: What shall the semantics be ?
                  //
                  //target.m_selector = source.m_selector;
                  target.m_selector = 0;

                  return true;
               }



               typedef common::platform::raw_buffer_type data_type;

               data_type handle()
               {
                  return payload.memory.data();
               }


            private:

               size_type m_selector = 0;

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
                  return result->handle();
               }

               common::platform::raw_buffer_type insert( common::buffer::Payload payload)
               {
                  m_pool.emplace_back( std::move( payload));
                  return m_pool.back().payload.memory.data();
               }

            };

         } //

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

            //struct trace : common::trace::internal::Scope
            struct trace
            {
               //explicit trace( std::string information) : Scope( std::move( information), common::log::internal::buffer) {}
               explicit trace( std::string information) {}
            };



            Buffer* find( const char* const handle)
            {
               const trace trace( "order::find");


               try
               {
                  auto& buffer = pool_type::pool.get( handle);

                  return &buffer;
               }
               catch( ...)
               {
                  //
                  // TODO: Perhaps have some dedicated order-logging ?
                  //
                  common::error::handler();
               }

               return nullptr;

            }

            int explore( const char* const handle, long* const size, long* const used)
            {
               const trace trace( "order::explore");

               if( const auto buffer = find( handle))
               {
                  if( size) *size = static_cast<long>(buffer->reserved());
                  if( used) *used = static_cast<long>(buffer->utilized());
               }
               else
               {
                  return CASUAL_ORDER_INVALID_BUFFER;
               }

               return CASUAL_ORDER_SUCCESS;

            }

            int copy( const char* const target_handle, const char* const source_handle)
            {
               const trace trace( "order::copy");

               const auto target = find( target_handle);

               const auto source = find( source_handle);

               if( target && source)
               {
                  if( Buffer::copy( *target, *source))
                  {
                     return CASUAL_ORDER_SUCCESS;
                  }
                  else
                  {
                     return CASUAL_ORDER_NO_SPACE;
                  }
               }
               else
               {
                  return CASUAL_ORDER_INVALID_BUFFER;
               }

            }

            int clear( const char* const handle)
            {
               const trace trace( "order::clear");

               if( auto buffer = find( handle))
               {
                  buffer->clear();
               }
               else
               {
                  return CASUAL_ORDER_INVALID_BUFFER;
               }

               return CASUAL_ORDER_SUCCESS;
            }

            int reset( const char* const handle)
            {
               const trace trace( "order::reset");

               if( auto buffer = find( handle))
               {
                  buffer->reset();
               }
               else
               {
                  return CASUAL_ORDER_INVALID_BUFFER;
               }

               return CASUAL_ORDER_SUCCESS;

            }

            template<typename... A>
            int add( const char* const handle, A&&... arguments)
            {
               const trace trace( "order::add");

               if( auto buffer = find( handle))
               {
                  if( buffer->append( std::forward<A>( arguments)...))
                  {
                     return CASUAL_ORDER_SUCCESS;
                  }
                  else
                  {
                     return CASUAL_ORDER_NO_SPACE;
                  }
               }
               else
               {
                  return CASUAL_ORDER_INVALID_BUFFER;
               }

            }

            template<typename... A>
            int get( const char* const handle, A&&... arguments)
            {
               const trace trace( "order::get");

               if( auto buffer = find( handle))
               {
                  if( buffer->select( std::forward<A>( arguments)...))
                  {
                     return CASUAL_ORDER_SUCCESS;
                  }
                  else
                  {
                     return CASUAL_ORDER_NO_PLACE;
                  }
               }
               else
               {
                  return CASUAL_ORDER_INVALID_BUFFER;
               }

            }


         } //

      } // order

   } // buffer



} // casual



const char* CasualOrderDescription( const int code)
{
   switch( code)
   {
      case CASUAL_ORDER_SUCCESS:
         return "Success";
      case CASUAL_ORDER_NO_SPACE:
         return "No space";
      case CASUAL_ORDER_NO_PLACE:
         return "No place";
      case CASUAL_ORDER_INVALID_BUFFER:
         return "Invalid buffer";
      case CASUAL_ORDER_INVALID_ARGUMENT:
         return "Invalid argument";
      case CASUAL_ORDER_INTERNAL_FAILURE:
         return "Internal failure";
      default:
         return "Unknown code";
   }
}

int CasualOrderExploreBuffer( const char* buffer, long* size, long* used)
{
   return casual::buffer::order::explore( buffer, size, used);
}


int CasualOrderAddPrepare( char* const buffer)
{
   return casual::buffer::order::clear( buffer);
}

int CasualOrderAddBool( char* const buffer, const bool value)
{
   return casual::buffer::order::add( buffer, value);
}

int CasualOrderAddChar( char* const buffer, const char value)
{
   return casual::buffer::order::add( buffer, value);
}

int CasualOrderAddShort( char* const buffer, const short value)
{
   return casual::buffer::order::add( buffer, value);
}

int CasualOrderAddLong( char* const buffer, const long value)
{
   return casual::buffer::order::add( buffer, value);
}

int CasualOrderAddFloat( char* const buffer, const float value)
{
   return casual::buffer::order::add( buffer, value);
}

int CasualOrderAddDouble( char* const buffer, const double value)
{
   return casual::buffer::order::add( buffer, value);
}

int CasualOrderAddString( char* const buffer, const char* const value)
{
   return casual::buffer::order::add( buffer, value);
}

int CasualOrderAddBinary( char* const buffer, const char* const data, const long size)
{
   if( size < 0)
   {
      return CASUAL_ORDER_INVALID_ARGUMENT;
   }

   return casual::buffer::order::add( buffer, data, size);
}

int CasualOrderGetPrepare( char* const buffer)
{
   return casual::buffer::order::reset( buffer);
}

int CasualOrderGetBool( char* const buffer, bool* const value)
{
   return casual::buffer::order::get( buffer, *value);
}

int CasualOrderGetChar( char* const buffer, char* const value)
{
   return casual::buffer::order::get( buffer, *value);
}

int CasualOrderGetShort( char* const buffer, short* const value)
{
   return casual::buffer::order::get( buffer, *value);
}

int CasualOrderGetLong( char* const buffer, long* const value)
{
   return casual::buffer::order::get( buffer, *value);
}

int CasualOrderGetFloat( char* const buffer, float* const value)
{
   return casual::buffer::order::get( buffer, *value);
}

int CasualOrderGetDouble( char* const buffer, double* const value)
{
   return casual::buffer::order::get( buffer, *value);
}

int CasualOrderGetString( char* const buffer, const char** value)
{
   return casual::buffer::order::get( buffer, *value);
}

int CasualOrderGetBinary( char* const buffer, const char** data, long* const size)
{
   return casual::buffer::order::get( buffer, *data, *size);
}

int CasualOrderCopyBuffer( char* target, const char* source)
{
   return casual::buffer::order::copy( target, source);
}


