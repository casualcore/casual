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
#include "common/internal/trace.h"
#include "common/log.h"

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

            class Buffer : public common::buffer::Buffer
            {
            public:

               using common::buffer::Buffer::Buffer;

               typedef common::platform::binary_type::size_type size_type;


               //
               // TODO: This should be moved to the Allocator-interface
               //
               size_type size( const size_type user_size) const
               {
                  //
                  // Ignore user provided size and return past last insert
                  //
                  return m_inserter;
               }

               size_type reserved() const
               {
                  return payload.memory.size();
               }

               void reserved( const size_type size)
               {
                  payload.memory.resize( size);
               }

               size_type utilized() const
               {
                  return m_inserter;
               }

               void utilized( const size_type size)
               {
                  m_inserter = size;
               }


               size_type consumed() const
               {
                  return m_selector;
               }

               void consumed( const size_type size)
               {
                  m_selector = size;
               }

               typedef common::platform::raw_buffer_type data_type;

               data_type handle()
               {
                  return payload.memory.data();
               }


            private:

               size_type m_inserter = 0;
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
                  //
                  // This should not need to be implemented, but 'cause of some
                  // probably not standard conformant behavior in GCC where
                  // zero size results in that std::vector::data returns null
                  //

                  m_pool.emplace_back( type, size ? size : 1);
                  return m_pool.back().handle();
               }


               common::platform::raw_buffer_type reallocate( const common::platform::const_raw_buffer_type handle, const common::platform::binary_size_type size)
               {
                  const auto result = find( handle);

                  const auto used = result->utilized();

                  //
                  // User may shrink a buffer, but not smaller than what's used
                  //
                  result->reserved( size < used ? used : size);

                  return result->handle();
               }

               common::platform::raw_buffer_type insert( common::buffer::Payload payload)
               {
                  m_pool.emplace_back( std::move( payload));
                  m_pool.back().utilized( m_pool.back().reserved());
                  return m_pool.back().handle();
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

            struct trace : common::trace::internal::Scope
            {
               explicit trace( std::string information)
               : Scope( std::move( information), common::log::internal::buffer)
               {}
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
                  if( target->reserved() < source->utilized())
                  {
                     //
                     // Not enough space
                     //
                     return CASUAL_ORDER_NO_SPACE;
                  }

                  target->utilized( source->utilized());
                  // TODO: This shall perhaps be set to 0 ?
                  target->consumed( source->consumed());

                  //
                  // Copy the content
                  //

                  std::copy(
                     source->handle(),
                     source->handle() + source->utilized(),
                     target->handle());
               }
               else
               {
                  return CASUAL_ORDER_INVALID_BUFFER;
               }

               return CASUAL_ORDER_SUCCESS;

            }

            int reset_insert( const char* const handle)
            {
               const trace trace( "order::reset_insert");

               if( auto buffer = find( handle))
               {
                  buffer->utilized( 0);
               }
               else
               {
                  return CASUAL_ORDER_INVALID_BUFFER;
               }

               return CASUAL_ORDER_SUCCESS;
            }

            int reset_select( const char* const handle)
            {
               const trace trace( "order::reset_select");

               if( auto buffer = find( handle))
               {
                  buffer->consumed( 0);
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
               const trace trace( "order::add");

               if( auto buffer = find( handle))
               {
                  const auto count = common::network::byteorder::bytes<T>();
                  const auto total = count + buffer->utilized();

                  if( total > buffer->reserved())
                  {
                     return CASUAL_ORDER_NO_SPACE;
                  }

                  const auto encoded = common::network::byteorder::encode( value);
                  std::memcpy( buffer->handle() + buffer->utilized(), &encoded, count);
                  buffer->utilized( total);
               }
               else
               {
                  return CASUAL_ORDER_INVALID_BUFFER;
               }

               return CASUAL_ORDER_SUCCESS;
            }

            int add( const char* const handle, const char* const value)
            {
               const trace trace( "order::add");

               if( auto buffer = find( handle))
               {
                  const auto count = std::strlen( value) + 1;
                  const auto total = buffer->utilized() + count;

                  if( total > buffer->reserved())
                  {
                     return CASUAL_ORDER_NO_SPACE;
                  }

                  std::memcpy( buffer->handle() + buffer->utilized(), value, count);
                  buffer->utilized( total);
               }
               else
               {
                  return CASUAL_ORDER_INVALID_BUFFER;
               }

               return CASUAL_ORDER_SUCCESS;
            }

            int add( const char* const handle, const char* const data, const long size)
            {
               const trace trace( "order::add");

               if( auto buffer = find( handle))
               {
                  const auto count = common::network::byteorder::bytes<long>();
                  const auto total = buffer->utilized() + count + size;

                  if( total > buffer->reserved())
                  {
                     return CASUAL_ORDER_NO_SPACE;
                  }

                  const auto encoded = common::network::byteorder::encode( size);
                  std::memcpy( buffer->handle() + buffer->utilized(), &encoded, count);
                  buffer->utilized( buffer->utilized() + count);
                  std::memcpy( buffer->handle() + buffer->utilized(), data, size);
                  buffer->utilized( buffer->utilized() + size);
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
               const trace trace( "order::get");

               if( auto buffer = find( handle))
               {
                  const auto count = common::network::byteorder::bytes<T>();
                  const auto total = buffer->consumed() + count;

                  if( total > buffer->utilized())
                  {
                     return CASUAL_ORDER_NO_PLACE;
                  }

                  const auto where = buffer->handle() + buffer->consumed();

                  const auto encoded = *reinterpret_cast< const common::network::byteorder::type<T>*>( where);
                  value = common::network::byteorder::decode<T>( encoded);
                  buffer->consumed( total);
               }
               else
               {
                  return CASUAL_ORDER_INVALID_BUFFER;
               }

               return CASUAL_ORDER_SUCCESS;
            }


            int get( const char* const handle, const char*& value)
            {
               const trace trace( "order::get");

               if( auto buffer = find( handle))
               {
                  const auto where = buffer->handle() + buffer->consumed();
                  const auto count = std::strlen( where) + 1;
                  const auto total = buffer->consumed() + count;

                  if( total > buffer->utilized())
                  {
                     return CASUAL_ORDER_NO_PLACE;
                  }

                  value = where;
                  buffer->consumed( total);
               }
               else
               {
                  return CASUAL_ORDER_INVALID_BUFFER;
               }

               return CASUAL_ORDER_SUCCESS;
            }

            int get( const char* const handle, const char*& data, long& size)
            {
               const trace trace( "order::get");

               if( auto buffer = find( handle))
               {
                  const auto count = common::network::byteorder::bytes<long>();
                  const auto total = buffer->consumed() + count;
                  if( total > buffer->utilized())
                  {
                     return CASUAL_ORDER_NO_PLACE;
                  }
                  else
                  {
                     const auto where = buffer->handle() + buffer->consumed();
                     const auto encoded = *reinterpret_cast< const common::network::byteorder::type<long>*>( where);
                     const auto decoded = common::network::byteorder::decode<long>( encoded);
                     const auto total = buffer->consumed() + count + decoded;

                     if( total > buffer->utilized())
                     {
                        return CASUAL_ORDER_NO_PLACE;
                     }

                     size = decoded;
                     data = where + count;
                     buffer->consumed( total);
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


