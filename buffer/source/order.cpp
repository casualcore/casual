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

#include <cstring>

#include <algorithm>


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

            struct Buffer : common::buffer::Buffer
            {
               using common::buffer::Buffer::Buffer;

               typedef common::platform::binary_type::size_type size_type;

               size_type inserter = 0;
               size_type selector = 0;

               size_type size( const size_type user_size) const
               {
                  //
                  // Ignore user provided size and return past last insert
                  //
                  return inserter;
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
                  //
                  // This should not need to be implemented, but 'cause of some
                  // probably not standard conformant behavior in GCC where
                  // zero size results in that std::vector::data returns null
                  //

                  m_pool.emplace_back( type, size ? size : 1);
                  return m_pool.back().payload.memory.data();
               }


               common::platform::raw_buffer_type reallocate( const common::platform::const_raw_buffer_type handle, const common::platform::binary_size_type size)
               {
                  const auto result = find( handle);

                  if( result == std::end( m_pool))
                  {
                     // TODO: shouldn't this be an error (exception) ?
                     return nullptr;
                  }

                  const auto used = result->inserter;

                  //
                  // User may shrink a buffer, but not smaller than what's used
                  //
                  result->payload.memory.resize( size < used ? used : size);

                  return result->payload.memory.data();
               }

               common::platform::raw_buffer_type insert( common::buffer::Payload payload)
               {
                  m_pool.emplace_back( std::move( payload));
                  m_pool.back().inserter = m_pool.back().payload.memory.size();
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
            Buffer* find_buffer( const char* const handle)
            {
               try
               {
                  auto& buffer = pool_type::pool.get( handle);

                  if( buffer.payload.type.type != CASUAL_ORDER)
                  {
                     //
                     // TODO: This should be some generic check
                     //
                     // TODO: Shall this be logged ?
                     //
                  }
                  else
                  {
                     return &buffer;
                  }

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
               if( const auto buffer = find_buffer( handle))
               {
                  if( size) *size = static_cast<long>(buffer->payload.memory.size());
                  if( used) *used = static_cast<long>(buffer->inserter);
               }
               else
               {
                  return CASUAL_ORDER_INVALID_BUFFER;
               }

               return CASUAL_ORDER_SUCCESS;

            }

            int copy( const char* const target_handle, const char* const source_handle)
            {
               const auto target = find_buffer( target_handle);

               const auto source = find_buffer( source_handle);

               if( target && source)
               {
                  if( target->payload.memory.size() < source->inserter)
                  {
                     //
                     // Not enough space
                     //
                     return CASUAL_ORDER_NO_SPACE;
                  }

                  target->inserter = source->inserter;
                  target->selector = source->selector;

                  //
                  // Copy the whole buffer (though perhaps not necessary)
                  //

                  std::copy(
                     source->payload.memory.begin(),
                     source->payload.memory.end(),
                     target->payload.memory.begin());
               }
               else
               {
                  return CASUAL_ORDER_INVALID_BUFFER;
               }

               return CASUAL_ORDER_SUCCESS;

            }

            int reset_insert( const char* const handle)
            {
               if( auto buffer = find_buffer( handle))
               {
                  buffer->inserter = 0;
               }
               else
               {
                  return CASUAL_ORDER_INVALID_BUFFER;
               }

               return CASUAL_ORDER_SUCCESS;
            }

            int reset_select( const char* const handle)
            {
               if( auto buffer = find_buffer( handle))
               {
                  buffer->selector = 0;
               }
               else
               {
                  return CASUAL_ORDER_INVALID_BUFFER;
               }

               return CASUAL_ORDER_SUCCESS;

            }

            //
            // TODO: Make 'add' and 'get' a lot more generic
            //


            template<typename T>
            int add( const char* const handle, const T value)
            {
               if( auto buffer = find_buffer( handle))
               {
                  const auto count = common::network::byteorder::bytes<T>();
                  const auto total = count + buffer->inserter;

                  if( total > buffer->payload.memory.size())
                  {
                     return CASUAL_ORDER_NO_SPACE;
                  }

                  const auto encoded = common::network::byteorder::encode( value);
                  std::memcpy( buffer->payload.memory.data() + buffer->inserter, &encoded, count);
                  buffer->inserter = total;
               }
               else
               {
                  return CASUAL_ORDER_INVALID_BUFFER;
               }

               return CASUAL_ORDER_SUCCESS;
            }

            int add( const char* const handle, const char* const value)
            {
               if( auto buffer = find_buffer( handle))
               {
                  const auto count = std::strlen( value) + 1;
                  const auto total = buffer->inserter + count;

                  if( total > buffer->payload.memory.size())
                  {
                     return CASUAL_ORDER_NO_SPACE;
                  }

                  std::memcpy( buffer->payload.memory.data() + buffer->inserter, value, count);
                  buffer->inserter = total;
               }
               else
               {
                  return CASUAL_ORDER_INVALID_BUFFER;
               }

               return CASUAL_ORDER_SUCCESS;
            }

            int add( const char* const handle, const char* const data, const long size)
            {
               if( auto buffer = find_buffer( handle))
               {
                  const auto count = common::network::byteorder::bytes<long>();
                  const auto total = buffer->inserter + count + size;

                  if( total > buffer->payload.memory.size())
                  {
                     return CASUAL_ORDER_NO_SPACE;
                  }

                  const auto encoded = common::network::byteorder::encode( size);
                  std::memcpy( buffer->payload.memory.data() + buffer->inserter, &encoded, count);
                  buffer->inserter += count;
                  std::memcpy( buffer->payload.memory.data() + buffer->inserter, data, size);
                  buffer->inserter += size;
               }
               else
               {
                  return CASUAL_ORDER_INVALID_BUFFER;
               }

               return CASUAL_ORDER_SUCCESS;
            }

            template<typename T>
            int get( const char* const handle, T& value)
            {
               if( auto buffer = find_buffer( handle))
               {
                  const auto count = common::network::byteorder::bytes<T>();
                  const auto total = buffer->selector + count;

                  if( total > buffer->inserter)
                  {
                     return CASUAL_ORDER_NO_PLACE;
                  }

                  const auto where = buffer->payload.memory.data() + buffer->selector;

                  const auto encoded = *reinterpret_cast< const common::network::byteorder::type<T>*>( where);
                  value = common::network::byteorder::decode<T>( encoded);
                  buffer->selector = total;
               }
               else
               {
                  return CASUAL_ORDER_INVALID_BUFFER;
               }

               return CASUAL_ORDER_SUCCESS;
            }


            int get( const char* const handle, const char*& value)
            {
               if( auto buffer = find_buffer( handle))
               {
                  const auto where = buffer->payload.memory.data() + buffer->selector;
                  const auto count = std::strlen( where) + 1;
                  const auto total = buffer->selector + count;

                  if( total > buffer->inserter)
                  {
                     return CASUAL_ORDER_NO_PLACE;
                  }

                  value = where;
                  buffer->selector = total;
               }
               else
               {
                  return CASUAL_ORDER_INVALID_BUFFER;
               }

               return CASUAL_ORDER_SUCCESS;
            }

            int get( const char* const handle, const char*& data, long& size)
            {
               if( auto buffer = find_buffer( handle))
               {
                  const auto count = common::network::byteorder::bytes<long>();
                  const auto total = buffer->selector + count;
                  if( total > buffer->inserter)
                  {
                     return CASUAL_ORDER_NO_PLACE;
                  }
                  else
                  {
                     const auto where = buffer->payload.memory.data() + buffer->selector;
                     const auto encoded = *reinterpret_cast< const common::network::byteorder::type<long>*>( where);
                     const auto decoded = common::network::byteorder::decode<long>( encoded);
                     const auto total = buffer->selector + count + decoded;

                     if( total > buffer->inserter)
                     {
                        return CASUAL_ORDER_NO_PLACE;
                     }

                     size = decoded;
                     data = where + count;
                     buffer->selector = total;
                  }
               }
               else
               {
                  return CASUAL_ORDER_INVALID_BUFFER;
               }

               return CASUAL_ORDER_SUCCESS;
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
   return casual::buffer::order::reset_insert( buffer);
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
   return casual::buffer::order::reset_select( buffer);
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


