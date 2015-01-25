//
// casual_order_buffer.cpp
//
//  Created on: 20 okt 2013
//      Author: Kristone
//

#include "buffer/order.h"

#include "common/buffer/pool.h"
#include "common/network_byteorder.h"
#include "common/platform.h"

#include "common/log.h"

#include <cstring>

#include <iostream>


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
          * since no validation of types or fields occurs what so ever. The
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
          * One doubtful decision is to (redundantly) store the allocated size
          * in the buffer as well that indeed simplifies a few things
          *
          * The buffer layout is |size|inserted|selected|data ...|
          *
          * The inserted-parameter is offset to where data was inserted
          * The selected-parameter is offset to where data was consumed
          *
          * The size, inserted, selected types are network-uint64_t
          *
          * String is stored with null-termination
          *
          * Binary is stored with a size (network-long) and then it's data
          *
          */

         namespace
         {
            typedef common::platform::const_raw_buffer_type const_data_type;
            typedef common::platform::raw_buffer_type data_type;

            typedef common::platform::binary_size_type size_type;

            // A thing to make stuff less bloaty ... not so good though
            constexpr auto size_size = common::network::byteorder::bytes<size_type>();

            //! Transform a value in place from network to host
            template<typename T>
            void parse( const_data_type where, T& value)
            {
               // Cast to network version
               const auto encoded = *reinterpret_cast< const common::network::byteorder::type<T>*>( where);
               // Decode to host version
               value = common::network::byteorder::decode<T>( encoded);
            }

            // TODO: We should remove this
            void parse( data_type where, data_type& value)
            {
               value = where;
            }

            // TODO: We should remove this
            void parse( const_data_type where, const_data_type& value)
            {
               value = where;
            }

            //! Transform a value in place from host to network
            template<typename T>
            void write( data_type where, const T value)
            {
               // Encode to network version
               const auto encoded = common::network::byteorder::encode( value);
               // Write it to buffer
               std::memcpy( where, &encoded, sizeof( encoded));
            }

            // TODO: Perhaps useless
            void write( data_type where, const_data_type value, const size_type count)
            {
               std::memcpy( where, value, count);
            }

            //!
            //! Helper-class to manage the buffer
            //!
            class Buffer
            {
            public:

               // Something to make other things less bloaty
               static constexpr auto size_size = common::network::byteorder::bytes<size_type>();

               template<typename type, size_type offset>
               class Data
               {
               public:

                  Data( data_type where) : m_where( where ? where + offset : nullptr) {}

                  //! @return Whether this is valid or not
                  explicit operator bool () const
                  {return m_where != nullptr;}

                  type operator() () const
                  {
                     type result;
                     parse( m_where, result);
                     return result;
                  }

                  void operator() ( const type value)
                  {
                     write( m_where, value);
                  }

               private:
                  data_type m_where;
               };

               Buffer( data_type buffer) :
                  size( buffer),
                  used( buffer),
                  pick( buffer),
                  data( buffer)
               {}

               // TODO: Can we make this more generic ?
               Data<size_type, size_size * 0> size;
               Data<size_type, size_size * 1> used;
               Data<size_type, size_size * 2> pick;
               Data<data_type, size_size * 3> data;

               // TODO: Make this more generic
               static constexpr size_type header()
               {return size_size + size_size + size_size;}

               //! @return Whether this buffer is valid or not
               explicit operator bool () const
               {return size ? true : false;}

               //! @return A 'handle' where to "insert"
               data_type inserter() const
               {
                  return data ? data() + used() - header() : nullptr;
               }

               //! @return A 'handle' where to "select"
               data_type selector() const
               {
                  return data ? data() + pick() - header() : nullptr;
               }

            };

            // TODO: Doesn't have to inherit from anything, but has to implement a set of functions (like pool::basic)
            class Allocator : public common::buffer::pool::default_pool
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
                  constexpr auto header = Buffer::header();

                  const auto actual = size < header ? header : size;

                  m_pool.emplace_back( type, actual);

                  auto data = m_pool.back().payload.memory.data();

                  Buffer buffer{ data};
                  buffer.size( actual);
                  buffer.used( header);
                  buffer.pick( header);

                  return data;
               }

               common::platform::raw_buffer_type reallocate( const common::platform::const_raw_buffer_type handle, const common::platform::binary_size_type size)
               {
                  const auto result = find( handle);

                  if( result == std::end( m_pool))
                  {
                     // TODO: shouldn't this be an error (exception) ?
                     return nullptr;
                  }

                  const auto used = Buffer( result->payload.memory.data()).used();

                  const auto actual = size < used ? used : size;

                  //
                  // User may shrink a buffer, but not smaller than what's used
                  //
                  result->payload.memory.resize( actual);

                  Buffer( result->payload.memory.data()).size( actual);

                  return result->payload.memory.data();
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
            Buffer find_buffer( const char* const handle)
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
                     return Buffer( buffer.payload.memory.data());
                  }

               }
               catch( ...)
               {
                  //
                  // TODO: Perhaps have some dedicated order-logging ?
                  //
                  common::error::handler();
               }

               return Buffer( nullptr);

            }

            int explore( const char* const handle, long* const size, long* const used)
            {
               const Buffer buffer = find_buffer( handle);

               if( buffer)
               {
                  if( size) *size = static_cast<long>(buffer.size());
                  if( used) *used = static_cast<long>(buffer.used());
               }
               else
               {
                  return CASUAL_ORDER_INVALID_BUFFER;
               }

               return CASUAL_ORDER_SUCCESS;

            }

            int reset_insert( const char* const handle)
            {
               Buffer buffer = find_buffer( handle);

               if( buffer)
               {
                  // TODO: Make this more ... clear
                  buffer.used( buffer.header());
               }
               else
               {
                  return CASUAL_ORDER_INVALID_BUFFER;
               }

               return CASUAL_ORDER_SUCCESS;
            }

            int reset_select( const char* const handle)
            {
               Buffer buffer = find_buffer( handle);

               if( buffer)
               {
                  // TODO: Make this more ... clear
                  buffer.pick( buffer.header());
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
               Buffer buffer = find_buffer( handle);

               if( !buffer)
               {
                  return CASUAL_ORDER_INVALID_BUFFER;
               }

               const auto count = common::network::byteorder::bytes<T>();
               const auto total = buffer.used() + count;

               if( total > buffer.size())
               {
                  return CASUAL_ORDER_NO_SPACE;
               }

               write( buffer.inserter(), value);

               buffer.used( total);

               return CASUAL_ORDER_SUCCESS;
            }

            int add( const char* const handle, const char* const value)
            {
               Buffer buffer = find_buffer( handle);

               if( !buffer)
               {
                  return CASUAL_ORDER_INVALID_BUFFER;
               }

               const auto count = std::strlen( value) + 1;

               const auto total = buffer.used() + count;

               if( total > buffer.size())
               {
                  return CASUAL_ORDER_NO_SPACE;
               }

               write( buffer.inserter(), value, count);

               buffer.used( total);

               return CASUAL_ORDER_SUCCESS;
            }

            int add( const char* const handle, const char* const value, const long count)
            {
               Buffer buffer = find_buffer( handle);

               if( !buffer)
               {
                  return CASUAL_ORDER_INVALID_BUFFER;
               }

               const auto total = buffer.used() + size_size + count;

               if( total > buffer.size())
               {
                  return CASUAL_ORDER_NO_SPACE;
               }

               write( buffer.inserter(), count);

               write( buffer.inserter() + size_size, value, count);

               buffer.used( total);

               return CASUAL_ORDER_SUCCESS;
            }

            template<typename T>
            int get( const char* const handle, T& value)
            {
               Buffer buffer = find_buffer( handle);

               if( !buffer)
               {
                  return CASUAL_ORDER_INVALID_BUFFER;
               }

               const auto count = common::network::byteorder::bytes<T>();

               const auto total = buffer.pick() + count;

               if( total > buffer.used())
               {
                  return CASUAL_ORDER_NO_PLACE;
               }

               parse( buffer.selector(), value);

               buffer.pick( total);

               return CASUAL_ORDER_SUCCESS;

            }


            int get( const char* const handle, const char*& value)
            {
               Buffer buffer = find_buffer( handle);

               if( !buffer)
               {
                  return CASUAL_ORDER_INVALID_BUFFER;
               }

               const auto count = std::strlen( buffer.selector()) + 1;

               const auto total = buffer.pick() + count;

               if( total > buffer.used())
               {
                  return CASUAL_ORDER_NO_PLACE;
               }

               parse( buffer.selector(), value);

               buffer.pick( total);

               return CASUAL_ORDER_SUCCESS;

            }

            int get( const char* const handle, const char*& value, long& count)
            {
               Buffer buffer = find_buffer( handle);

               if( !buffer)
               {
                  return CASUAL_ORDER_INVALID_BUFFER;
               }

               // We start by reading the size (unconditionally)
               parse( buffer.selector(), count);

               const auto total = buffer.pick() + size_size + count;

               if( total > buffer.used())
               {
                  return CASUAL_ORDER_NO_PLACE;
               }

               parse( buffer.selector() + size_size, value);

               buffer.pick( total);

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
   long used;
   if( const auto result = casual::buffer::order::explore( source, nullptr, &used))
   {
      return result;
   }

   long size;
   if( const auto result = casual::buffer::order::explore( target, &size, nullptr))
   {
      return result;
   }

   if( size < used)
   {
      return CASUAL_ORDER_NO_SPACE;
   }

   //
   // TODO: This is a little bit too much knowledge 'bout internal stuff
   //
   // This is to make sure the size-portion is not blown away
   //
   target += casual::buffer::order::size_size;
   source += casual::buffer::order::size_size;

   casual::buffer::order::write( target, source, used);

   return CASUAL_ORDER_SUCCESS;
}


