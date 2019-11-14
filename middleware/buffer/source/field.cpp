//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "casual/buffer/field.h"
#include "casual/buffer/internal/field.h"
#include "casual/buffer/internal/common.h"
#include "casual/buffer/internal/field/string.h"

#include "common/environment.h"
#include "common/exception/xatmi.h"
#include "common/exception/handle.h"
#include "common/network/byteorder.h"
#include "common/buffer/pool.h"
#include "common/buffer/type.h"
#include "common/log.h"
#include "casual/platform.h"
#include "common/algorithm.h"
#include "common/transcode.h"
#include "common/execute.h"

#include "common/serialize/create.h"



//#include "common/serialize/create.h"

#include <cstring>

// std
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <iterator>
#include <regex>
#include <iostream>
#include <sstream>

namespace casual
{
   using namespace common;

   namespace buffer
   {
      namespace field
      {

         namespace
         {
            using item_type = long;
            using size_type = platform::binary::size::type;
            using const_data_type = platform::binary::type::const_pointer;
            using data_type = platform::binary::type::pointer;


            template<typename T>
            T decode( const_data_type where) noexcept
            {
               using network_type = common::network::byteorder::type<T>;
               const auto encoded = *reinterpret_cast< const network_type*>( where);
               return common::network::byteorder::decode<T>( encoded);
            }


            enum : long
            {
               item_offset = 0,
               size_offset = item_offset + common::network::byteorder::bytes<item_type>(),
               data_offset = size_offset + common::network::byteorder::bytes<size_type>(),
            };

            struct Buffer : common::buffer::Buffer
            {
               std::map<item_type,std::vector<size_type>> index;

               template<typename... A>
               Buffer( A&&... arguments) : common::buffer::Buffer( std::forward<A>( arguments)...)
               {
                  // Create an index upon creation

                  const auto begin = payload.memory.begin();
                  const auto end = payload.memory.end();
                  auto cursor = begin;

                  while( cursor < end)
                  {
                     const auto item = decode<item_type>( &*cursor + item_offset);
                     const auto size = decode<size_type>( &*cursor + size_offset);

                     index[item].push_back( std::distance( begin, cursor));

                     std::advance( cursor, data_offset + size);
                  }

                  if( cursor > end)
                     throw common::exception::xatmi::invalid::Argument{"Buffer is comprised"};
               }

               void shrink()
               {
                  return payload.memory.shrink_to_fit();
               }

               size_type capacity() const noexcept
               {
                  return payload.memory.capacity();
               }

               void capacity( const decltype(payload.memory.capacity()) value)
               {
                  payload.memory.reserve( value);
               }


               size_type utilized() const noexcept
               {
                  return payload.memory.size();
               }

               void utilized( const decltype(payload.memory.size()) value)
               {
                  payload.memory.resize( value);
               }

               auto handle() const noexcept
               {
                  return payload.memory.data();
               }

               auto handle() noexcept
               {
                  return payload.memory.data();
               }

               //! Implement Buffer::transport
               size_type transport( const platform::binary::size::type user_size) const
               {
                  // Just ignore user-size all together

                  return utilized();
               }

               //! Implement Buffer::reserved
               size_type reserved() const
               {
                  return capacity();
               }

            };

            // Might be named Pool as well
            class Allocator : public common::buffer::pool::basic_pool<Buffer>
            {
            public:

               using types_type = common::buffer::pool::default_pool::types_type;

               static const types_type& types()
               {
                  // The types this pool can manage
                  static const types_type result{ common::buffer::type::combine( CASUAL_FIELD)};
                  return result;
               }

               platform::buffer::raw::type allocate( const std::string& type, const platform::binary::size::type size)
               {
                  m_pool.emplace_back( type, 0);

                  // GCC returns null for std::vector::data with capacity zero
                  m_pool.back().capacity( size ? size : 1);

                  common::log::line( verbose::log, "allocated buffer: ", m_pool.back());

                  return m_pool.back().handle();
               }

               platform::buffer::raw::type reallocate( const platform::buffer::raw::immutable::type handle, const platform::binary::size::type size)
               {
                  const auto result = find( handle);

                  if( size < result->utilized())
                  {
                     // Allow user to reduce allocation
                     result->shrink();
                  }
                  else
                  {
                     // GCC returns null for std::vector::data with size zero
                     result->capacity( size ? size : 1);
                  }

                  common::log::line( verbose::log, "reallocated buffer: ", *result);

                  return result->handle();
               }

            };
         } // <unnamed>
      } // field
   } // buffer


   // Register and define the type that can be used to get the custom pool
   template class common::buffer::pool::Registration< casual::buffer::field::Allocator>;


   namespace buffer
   {
      namespace field
      {

         using pool_type = common::buffer::pool::Registration< Allocator>;

         namespace
         {

            namespace error
            {
               int handle() noexcept
               {
                  try
                  {
                     throw;
                  }
                  catch( const std::out_of_range&)
                  {
                     return CASUAL_FIELD_OUT_OF_BOUNDS;
                  }
                  catch( const std::invalid_argument&)
                  {
                     return CASUAL_FIELD_INVALID_ARGUMENT;
                  }
                  catch( const std::bad_alloc&)
                  {
                     return CASUAL_FIELD_OUT_OF_MEMORY;
                  }
                  catch( const common::exception::xatmi::invalid::Argument&)
                  {
                     return CASUAL_FIELD_INVALID_HANDLE;
                  }
                  catch( ...)
                  {
                     common::exception::handle();
                     return CASUAL_FIELD_INTERNAL_FAILURE;
                  }
               }
            }

            namespace add
            {

               template<typename B>
               void append( B& buffer, const_data_type data, const std::size_t size)
               {
                  buffer.payload.memory.insert( buffer.payload.memory.end(), data, data + size);
               }


               template<typename B, typename T>
               void append( B& buffer, const T value)
               {
                  const auto encoded = common::network::byteorder::encode( value);
                  append( buffer, reinterpret_cast<const_data_type>( &encoded), sizeof( encoded));
               }


               template<typename B>
               void append( B& buffer, const item_type id, const_data_type data, const size_type size)
               {
                  const auto used = buffer.utilized();

                  try
                  {
                     // Append id to buffer
                     append( buffer, id);

                     // Append size to buffer
                     append( buffer, size);

                     // Append data to buffer
                     append( buffer, data, size);

                     // Append current offset to index
                     buffer.index[id].push_back( used);

                  }
                  catch( ...)
                  {
                     // Make sure to reset the size in case of exception
                     buffer.utilized( used);
                     throw;
                  }
               }

               template<typename B>
               void append( B& buffer, const item_type id, const char* const value)
               {
                  append( buffer, id, value, std::strlen( value) + 1);
               }

               template<typename B, typename T>
               void append( B& buffer, const item_type id, const T value)
               {
                  const auto encoded = common::network::byteorder::encode( value);
                  append( buffer, id, reinterpret_cast<const_data_type>( &encoded), sizeof( encoded));
               }

               template<typename... A>
               int data( char** handle, const item_type id, const int type, A&&... arguments)
               {
                  //const trace trace( "field::add::data");

                  if( type != (id / CASUAL_FIELD_TYPE_BASE))
                  {

                     log::line( casual::buffer::verbose::log, "buffer::field::add::data: invalid argument - id: ", id, " - type: ", type);
                     return CASUAL_FIELD_INVALID_ARGUMENT;
                  }

                  try
                  {
                     auto& buffer = pool_type::pool.get( *handle);

                     // Make sure to update the handle regardless
                     const auto synchronize = common::execute::scope
                     ( [&]() { *handle = buffer.handle();});

                     // Append the data
                     append( buffer, id, std::forward<A>( arguments)...);

                  }
                  catch( ...)
                  {
                     return error::handle();
                  }

                  return CASUAL_FIELD_SUCCESS;

               }
            } // add

            namespace get
            {
               template<typename I>
               size_type offset( const I& index, const item_type id, const size_type occurrence)
               {
                  return index.at( id).at( occurrence);
               }

               template<typename B>
               void select( const B& buffer, const item_type id, const size_type occurrence, const_data_type& data, size_type& size)
               {
                  const auto field_offset = offset( buffer.index, id, occurrence);

                  data = buffer.handle() + field_offset + data_offset;

                  size = decode<size_type>( buffer.handle() + field_offset + size_offset);
               }

               template<typename B>
               void select( const B& buffer, const item_type id, const size_type occurrence, const_data_type& data)
               {
                  const auto field_offset = offset( buffer.index, id, occurrence);

                  data = buffer.handle() + field_offset + data_offset;
               }

               template<typename B, typename T>
               void select( const B& buffer, const item_type id, const size_type occurrence, T& data)
               {
                  const auto field_offset = offset( buffer.index, id, occurrence);

                  data = decode<T>( buffer.handle() + field_offset + data_offset);
               }


               template<typename... A>
               int data( const char* const handle, const item_type id, size_type occurrence, const int type, A&&... arguments)
               {
                  //const trace trace( "field::get::data");

                  if( type != (id / CASUAL_FIELD_TYPE_BASE))
                  {
                     return CASUAL_FIELD_INVALID_ARGUMENT;
                  }

                  try
                  {
                     const auto& buffer = pool_type::pool.get( handle);

                     // Select the data
                     select( buffer, id, occurrence, std::forward<A>( arguments)...);

                  }
                  catch( ...)
                  {
                     return error::handle();
                  }

                  return CASUAL_FIELD_SUCCESS;

               }

            } // get

            namespace cut
            {

               template<typename B>
               void remove( B& buffer, const item_type id, const size_type occurrence)
               {
                  auto& occurrences = buffer.index.at( id);

                  const auto offset = occurrences.at( occurrence);

                  // Get the size of the value
                  const auto size = decode<size_type>( buffer.handle() + offset + size_offset);

                  // Remove the data from the buffer
                  buffer.payload.memory.erase(
                     buffer.payload.memory.begin() + offset,
                     buffer.payload.memory.begin() + offset + data_offset + size);

                  // Remove entry from index

                  occurrences.erase( occurrences.begin() + occurrence);

                  if( occurrences.empty())
                  {
                     buffer.index.erase( id);
                  }

                  // Update offsets beyond this one
                  //
                  // The index and the actual buffer may not be in same order
                  // and thus we need to go through the whole index

                  for( auto& field : buffer.index)
                  {
                     for( auto& occurrence : field.second)
                     {
                        if( occurrence > offset)
                        {
                           occurrence += (0 - data_offset - size);
                        }
                     }
                  }

               }


               int data( const char* const handle, const item_type id, size_type occurrence)
               {
                  //const trace trace( "field::cut::data");

                  if( ! (id > CASUAL_FIELD_NO_ID))
                  {
                     return CASUAL_FIELD_INVALID_ARGUMENT;
                  }

                  try
                  {
                     auto& buffer = pool_type::pool.get( handle);

                     remove( buffer, id, occurrence);
                  }
                  catch( ...)
                  {
                     return error::handle();
                  }

                  return CASUAL_FIELD_SUCCESS;

               }

               int all( const char* const handle)
               {
                  //const trace trace( "field::cut::all");

                  try
                  {
                     auto& buffer = pool_type::pool.get( handle);

                     buffer.payload.memory.clear();
                     buffer.index.clear();
                  }
                  catch( ...)
                  {
                     return error::handle();
                  }

                  return CASUAL_FIELD_SUCCESS;

               }

            } // cut

            namespace set
            {

               template<typename M, typename I>
               void update( M& memory, I& index, const item_type id, const size_type occurrence, const_data_type data, const size_type size)
               {
                  // Get the offset to where the field begin
                  const auto offset = index.at( id).at( occurrence);

                  const auto current = decode<size_type>( memory.data() + offset + size_offset);

                  // With equal sizes, stuff could be optimized ... but no

                  // Erase the old and insert the new ... and yes, it
                  // could be done more efficient but the whole idea
                  // with this functionality is rather stupid

                  memory.insert(
                     memory.erase(
                        memory.begin() + offset + data_offset,
                        memory.begin() + offset + data_offset + current),
                     data, data + size);

                  // Write the new size (afterwards since above can throw)

                  const auto encoded = common::network::byteorder::encode( size);
                  std::copy(
                     reinterpret_cast<const_data_type>( &encoded),
                     reinterpret_cast<const_data_type>( &encoded) + sizeof( encoded),
                     memory.begin() + offset + size_offset);

                  // Update offsets beyond this one
                  //
                  // The index and the actual buffer may not be in same order
                  // and thus we need to go through the whole index

                  for( auto& field : index)
                  {
                     for( auto& occurrence : field.second)
                     {
                        if( occurrence > offset)
                        {
                           occurrence += (size - current);
                        }
                     }
                  }

               }

               template<typename M, typename I>
               void update( M& memory, I& index, const item_type id, const size_type occurrence, const_data_type value)
               {
                  const auto count = std::strlen( value) + 1;
                  update( memory, index, id, occurrence, value, count);
               }


               template<typename M, typename I, typename T>
               void update( M& memory, I& index, const item_type id, const size_type occurrence, const T value)
               {
                  // Could be optimized, but ... no
                  const auto encoded = common::network::byteorder::encode( value);
                  update( memory, index, id, occurrence, reinterpret_cast<const_data_type>( &encoded), sizeof( encoded));
               }



               template<typename... A>
               int data( char** handle, const item_type id, size_type occurrence, const int type, A&&... arguments)
               {
                  //const trace trace( "field::set::data");

                  if( type != (id / CASUAL_FIELD_TYPE_BASE))
                  {
                     return CASUAL_FIELD_INVALID_ARGUMENT;
                  }

                  try
                  {
                     auto& buffer = pool_type::pool.get( *handle);

                     // Make sure to update the handle regardless
                     const auto synchronize = common::execute::scope( [&]() 
                     { 
                        *handle = buffer.handle();
                     });

                     // Update the data
                     update( buffer.payload.memory, buffer.index, id, occurrence, std::forward<A>( arguments)...);

                  }
                  catch( ...)
                  {
                     return error::handle();
                  }

                  return CASUAL_FIELD_SUCCESS;

               }

            } // set

            namespace explore
            {
               int value( const char* const handle, const item_type id, const size_type occurrence, size_type& count)
               {
                  //const trace trace( "field::explore::value");

                  try
                  {
                     const auto& buffer = pool_type::pool.get( handle);

                     const auto offset = buffer.index.at( id).at( occurrence);

                     count = decode<size_type>( buffer.payload.memory.data() + offset + size_offset);
                  }
                  catch( ...)
                  {
                     return error::handle();
                  }

                  return CASUAL_FIELD_SUCCESS;
               }

               int buffer( const char* const handle, size_type& size, size_type& used)
               {
                  //const trace trace( "field::explore::buffer");

                  try
                  {
                     const auto& buffer = pool_type::pool.get( handle);
                     size = buffer.capacity();
                     used = buffer.utilized();
                  }
                  catch( ...)
                  {
                     return error::handle();
                  }

                  return CASUAL_FIELD_SUCCESS;
               }

               int existence( const char* const handle, const item_type id, const size_type occurrence)
               {
                  //const trace trace( "field::explore::existence");

                  try
                  {
                     const auto& buffer = pool_type::pool.get( handle);

                     // Just to force an exception
                     buffer.index.at( id).at( occurrence);

                     return CASUAL_FIELD_SUCCESS;
                  }
                  catch( ...)
                  {
                     return error::handle();
                  }

               }


               int count( const char* const handle, const item_type id, size_type& occurrences)
               {
                  //const trace trace( "field::explore::count");

                  try
                  {
                     const auto& buffer = pool_type::pool.get( handle);

                     auto found = common::algorithm::find( buffer.index, id);

                     if( found)
                        occurrences = found->second.size();
                     else
                        occurrences = 0;
                  }
                  catch( ...)
                  {
                     return error::handle();
                  }

                  return CASUAL_FIELD_SUCCESS;
               }

               int count( const char* const handle, size_type& occurrences)
               {
                  //const trace trace( "field::explore::count");

                  try
                  {
                     const auto& buffer = pool_type::pool.get( handle);

                     occurrences =
                        std::accumulate(
                           buffer.index.begin(), buffer.index.end(), 0,
                           []( const auto count, const auto& field)
                           { return count + field.second.size();});
                  }
                  catch( ...)
                  {
                     return error::handle();
                  }

                  return CASUAL_FIELD_SUCCESS;
               }

            } // explore

            namespace iterate
            {
               int first( const char* const handle, item_type& id, size_type& index)
               {
                  //const trace trace( "field::iterate::first");

                  try
                  {
                     const auto& buffer = pool_type::pool.get( handle);

                     if( buffer.index.empty())
                     {
                        return CASUAL_FIELD_OUT_OF_BOUNDS;
                     }

                     id = buffer.index.begin()->first;
                     index = 0;
                  }
                  catch( ...)
                  {
                     return error::handle();
                  }

                  return CASUAL_FIELD_SUCCESS;

               }

               int next( const char* const handle, item_type& id, size_type& index)
               {
                  //const trace trace( "field::iterate::next");

                  try
                  {
                     const auto& buffer = pool_type::pool.get( handle);

                     const auto current = buffer.index.find( id);
                     if( current != buffer.index.end())
                     {
                        if( static_cast<std::size_t>(++index) < current->second.size())
                        {
                           // Then we are ok
                        }
                        else
                        {
                           const auto next = std::next( current);
                           if( next != buffer.index.end())
                           {
                              id = next->first;
                              index = 0;
                           }
                           else
                           {
                              return CASUAL_FIELD_OUT_OF_BOUNDS;
                           }
                        }
                     }
                     else
                     {
                        // We couldn't even find the previous one
                        return CASUAL_FIELD_INVALID_ARGUMENT;
                     }

                  }
                  catch( ...)
                  {
                     return error::handle();
                  }

                  return CASUAL_FIELD_SUCCESS;

               }

            } // iterate

            namespace copy
            {
               int buffer( char** target_handle, const char* const source_handle)
               {
                  //const trace trace( "field::copy::buffer");


                  try
                  {
                     auto& target = pool_type::pool.get( *target_handle);
                     const auto& source = pool_type::pool.get( source_handle);

                     const auto synchronize = common::execute::scope
                     ( [&]() { *target_handle = target.handle();});

                     auto index = source.index;
                     target.payload.memory = source.payload.memory;
                     std::swap( target.index, index);
                  }
                  catch( ...)
                  {
                     return error::handle();
                  }

                  return CASUAL_FIELD_SUCCESS;
               }

               int memory( char** const handle, const void* const source, const platform::binary::size::type count)
               {
                  //const trace trace( "field::copy::data");

                  try
                  {
                     auto& buffer = pool_type::pool.get( *handle);

                     const auto synchronize = common::execute::scope
                     ( [&]() { *handle = buffer.handle();});

                     const auto data = static_cast<const_data_type>(source);
                     const auto size = count;

                     buffer = common::buffer::Payload{ buffer.payload.type, { data, data + size}};
                  }
                  catch( ...)
                  {
                     return error::handle();
                  }

                  return CASUAL_FIELD_SUCCESS;
               }

            } // copy

         } // <unnamed>

      } // field

   } // buffer

} // casual


const char* casual_field_description( const int code)
{
   switch( code)
   {
      case CASUAL_FIELD_SUCCESS:
         return "Success";
      case CASUAL_FIELD_INVALID_HANDLE:
         return "Invalid handle";
      case CASUAL_FIELD_INVALID_ARGUMENT:
         return "Invalid argument";
      case CASUAL_FIELD_OUT_OF_MEMORY:
         return "Out of memory";
      case CASUAL_FIELD_OUT_OF_BOUNDS:
         return "Out of bounds";
      case CASUAL_FIELD_INTERNAL_FAILURE:
         return "Internal failure";
      default:
         return "Unknown code";
   }
}

int casual_field_explore_buffer( const char* buffer, long* const size, long* const used)
{
   long reserved{};
   long utilized{};
   if( const auto result = casual::buffer::field::explore::buffer( buffer, reserved, utilized))
   {
      return result;
   }

   if( size) *size = reserved;
   if( used) *used = utilized;

   return CASUAL_FIELD_SUCCESS;
}

int casual_field_explore_value( const char* const buffer, const long id, const long index, long* const count)
{
   if( count)
   {
      const auto type = id / CASUAL_FIELD_TYPE_BASE;

      switch( type)
      {
      case CASUAL_FIELD_STRING:
      case CASUAL_FIELD_BINARY:
         return casual::buffer::field::explore::value( buffer, id, index, *count);
      default:
         break;
      }

      if( const auto result = casual_field_plain_type_host_size( type, count))
      {
         return result;
      }

   }

   return casual::buffer::field::explore::existence( buffer, id, index);

}

int casual_field_occurrences_of_id( const char* const buffer, const long id, long* const occurrences)
{
   if( occurrences && id != CASUAL_FIELD_NO_ID)
   {
      return casual::buffer::field::explore::count( buffer, id, *occurrences);
   }

   casual::common::log::line( casual::buffer::verbose::log, "casual_field_occurrences_of_id: invalid argument - id: ", id, " - occurrences: ", occurrences);
   return CASUAL_FIELD_INVALID_ARGUMENT;
}

int casual_field_occurrences_in_buffer( const char* const buffer, long* const occurrences)
{
   if( occurrences)
   {
      return casual::buffer::field::explore::count( buffer, *occurrences);
   }

   return CASUAL_FIELD_INVALID_ARGUMENT;
}

int casual_field_add_char( char** const buffer, const long id, const char value)
{
   return casual::buffer::field::add::data( buffer, id, CASUAL_FIELD_CHAR, value);
}

int casual_field_add_short( char** const buffer, const long id, const short value)
{
   return casual::buffer::field::add::data( buffer, id, CASUAL_FIELD_SHORT, value);
}

int casual_field_add_long( char** const buffer, const long id, const long value)
{
   return casual::buffer::field::add::data( buffer, id, CASUAL_FIELD_LONG, value);
}
int casual_field_add_float( char** const buffer, const long id, const float value)
{
   return casual::buffer::field::add::data( buffer, id, CASUAL_FIELD_FLOAT, value);
}

int casual_field_add_double( char** const buffer, const long id, const double value)
{
   return casual::buffer::field::add::data( buffer, id, CASUAL_FIELD_DOUBLE, value);
}

int casual_field_add_string( char** const buffer, const long id, const char* const value)
{
   return casual::buffer::field::add::data( buffer, id, CASUAL_FIELD_STRING, value);
}

int casual_field_add_binary( char** const buffer, const long id, const char* const value, const long count)
{
   if( count < 0)
   {
      return CASUAL_FIELD_INVALID_ARGUMENT;
   }

   return casual::buffer::field::add::data( buffer, id, CASUAL_FIELD_BINARY, value, count);
}

int casual_field_add_value( char** const buffer, const long id, const void* const value, const long count)
{

   switch( id / CASUAL_FIELD_TYPE_BASE)
   {
   case CASUAL_FIELD_SHORT:
      return casual_field_add_short ( buffer, id, *static_cast<const short*>( value));
   case CASUAL_FIELD_LONG:
      return casual_field_add_long  ( buffer, id, *static_cast<const long*>( value));
   case CASUAL_FIELD_CHAR:
      return casual_field_add_char  ( buffer, id, *static_cast<const char*>( value));
   case CASUAL_FIELD_FLOAT:
      return casual_field_add_float ( buffer, id, *static_cast<const float*>( value));
   case CASUAL_FIELD_DOUBLE:
      return casual_field_add_double( buffer, id, *static_cast<const double*>( value));
   case CASUAL_FIELD_STRING:
      return casual_field_add_string( buffer, id, static_cast<const char*>( value));
   case CASUAL_FIELD_BINARY:
      return casual_field_add_binary( buffer, id, static_cast<const char*>( value), count);
   default:
      return CASUAL_FIELD_INVALID_ARGUMENT;
   }

}

int casual_field_add_empty( char** const buffer, const long id)
{
   switch( id / CASUAL_FIELD_TYPE_BASE)
   {
   case CASUAL_FIELD_SHORT:
      return casual_field_add_short ( buffer, id, 0);
   case CASUAL_FIELD_LONG:
      return casual_field_add_long  ( buffer, id, 0);
   case CASUAL_FIELD_CHAR:
      return casual_field_add_char  ( buffer, id, '\0');
   case CASUAL_FIELD_FLOAT:
      return casual_field_add_float ( buffer, id, 0.0);
   case CASUAL_FIELD_DOUBLE:
      return casual_field_add_double( buffer, id, 0.0);
   case CASUAL_FIELD_STRING:
      return casual_field_add_string( buffer, id, "");
   case CASUAL_FIELD_BINARY:
      return casual_field_add_binary( buffer, id, "", 0);
   default:
      return CASUAL_FIELD_INVALID_ARGUMENT;
   }

}


int casual_field_get_char( const char* const buffer, const long id, const long index, char* const value)
{
   return casual::buffer::field::get::data( buffer, id, index, CASUAL_FIELD_CHAR, *value);
}

int casual_field_get_short( const char* const buffer, const long id, const long index, short* const value)
{
   return casual::buffer::field::get::data( buffer, id, index, CASUAL_FIELD_SHORT, *value);
}

int casual_field_get_long( const char* const buffer, const long id, const long index, long* const value)
{
   return casual::buffer::field::get::data( buffer, id, index, CASUAL_FIELD_LONG, *value);
}

int casual_field_get_float( const char* const buffer, const long id, const long index, float* const value)
{
   return casual::buffer::field::get::data( buffer, id, index, CASUAL_FIELD_FLOAT, *value);
}

int casual_field_get_double( const char* const buffer, const long id, const long index, double* const value)
{
   return casual::buffer::field::get::data( buffer, id, index, CASUAL_FIELD_DOUBLE, *value);
}

int casual_field_get_string( const char* const buffer, const long id, const long index, const char** value)
{
   return casual::buffer::field::get::data( buffer, id, index, CASUAL_FIELD_STRING, *value);
}

int casual_field_get_binary( const char* const buffer, const long id, const long index, const char** value, long* const count)
{
   return casual::buffer::field::get::data( buffer, id, index, CASUAL_FIELD_BINARY, *value, *count);
}

int casual_field_get_value( const char* const buffer, const long id, const long index, void* const value, long* const count)
{
   if( ! value)
   {
      return casual_field_explore_value( buffer, id, index, count);
   }

   const int type = id / CASUAL_FIELD_TYPE_BASE;

   const char* data = nullptr;
   long size = 0;

   switch( type)
   {
   case CASUAL_FIELD_STRING:
   case CASUAL_FIELD_BINARY:
      if( const auto result = casual::buffer::field::get::data( buffer, id, index, type, data, size))
         return result;
      break;
   default:
      if( const auto result = casual_field_plain_type_host_size( type, &size))
         return result;
      break;
   }

   if( count)
   {
      if( *count < size)
      {
         casual::common::log::line( casual::buffer::verbose::log, "casual_field_get_value: invalid argument - id: ", id, 
            " - index: ", index, " - count: ", *count,
            " - size: ", size);
         return CASUAL_FIELD_INVALID_ARGUMENT;
      }


      //
      // This is perhaps not Fget32-compatible if field is invalid
      //
      *count = size;
   }

   switch( type)
   {
   case CASUAL_FIELD_SHORT:
      return casual_field_get_short( buffer, id, index, static_cast<short*>( value));
   case CASUAL_FIELD_LONG:
      return casual_field_get_long( buffer, id, index, static_cast<long*>( value));
   case CASUAL_FIELD_CHAR:
      return casual_field_get_char( buffer, id, index, static_cast<char*>( value));
   case CASUAL_FIELD_FLOAT:
      return casual_field_get_float( buffer, id, index, static_cast<float*>( value));
   case CASUAL_FIELD_DOUBLE:
      return casual_field_get_double( buffer, id, index, static_cast<double*>( value));
   default:
      break;
   }

   std::memcpy( value, data, size);

   return CASUAL_FIELD_SUCCESS;

}

int casual_field_set_char( char** const buffer, const long id, const long index, const char value)
{
   return casual::buffer::field::set::data( buffer, id, index, CASUAL_FIELD_CHAR, value);
}

int casual_field_set_short( char** const buffer, const long id, const long index, const short value)
{
   return casual::buffer::field::set::data( buffer, id, index, CASUAL_FIELD_SHORT, value);
}

int casual_field_set_long( char** const buffer, const long id, const long index, const long value)
{
   return casual::buffer::field::set::data( buffer, id, index, CASUAL_FIELD_LONG, value);
}

int casual_field_set_float( char** const buffer, const long id, const long index, const float value)
{
   return casual::buffer::field::set::data( buffer, id, index, CASUAL_FIELD_FLOAT, value);
}

int casual_field_set_double( char** const buffer, const long id, const long index, const double value)
{
   return casual::buffer::field::set::data( buffer, id, index, CASUAL_FIELD_DOUBLE, value);
}

int casual_field_set_string( char** const buffer, const long id, const long index, const char* const value)
{
   if( value)
   {
      return casual::buffer::field::set::data( buffer, id, index, CASUAL_FIELD_STRING, value);
   }

   return CASUAL_FIELD_INVALID_ARGUMENT;
}

int casual_field_set_binary( char** const buffer, const long id, const long index, const char* const value, const long count)
{
   if( count < 0)
   {
      return CASUAL_FIELD_INVALID_ARGUMENT;
   }

   return casual::buffer::field::set::data( buffer, id, index, CASUAL_FIELD_BINARY, value, count);
}

int casual_field_set_value( char** const buffer, const long id, const long index, const void* const value, const long count)
{
   if( ! value)
   {
      return CASUAL_FIELD_INVALID_ARGUMENT;
   }

   switch( id / CASUAL_FIELD_TYPE_BASE)
   {
   case CASUAL_FIELD_SHORT:
      return casual_field_set_short ( buffer, id, index, *static_cast<const short*>( value));
   case CASUAL_FIELD_LONG:
      return casual_field_set_long  ( buffer, id, index, *static_cast<const long*>( value));
   case CASUAL_FIELD_CHAR:
      return casual_field_set_char  ( buffer, id, index, *static_cast<const char*>( value));
   case CASUAL_FIELD_FLOAT:
      return casual_field_set_float ( buffer, id, index, *static_cast<const float*>( value));
   case CASUAL_FIELD_DOUBLE:
      return casual_field_set_double( buffer, id, index, *static_cast<const double*>( value));
   case CASUAL_FIELD_STRING:
      return casual_field_set_string( buffer, id, index, static_cast<const char*>( value));
   case CASUAL_FIELD_BINARY:
      return casual_field_set_binary( buffer, id, index, static_cast<const char*>( value), count);
   default:
      return CASUAL_FIELD_INVALID_ARGUMENT;
   }

}

int casual_field_put_value( char** buffer, const long id, const long index, const void* const value, const long count)
{
   if( index < 0)
   {
      return casual_field_add_value( buffer, id, value, count);
   }

   //
   // Can be more efficient but remember to add empty occurrences if needed
   //

   const auto result = casual_field_set_value( buffer, id, index, value, count);

   if( result == CASUAL_FIELD_OUT_OF_BOUNDS)
   {
      if( const auto result = casual_field_add_empty( buffer, id))
      {
         return result;
      }

      return casual_field_put_value( buffer, id, index, value, count);

   }

   return result;

}




namespace casual
{
   namespace buffer
   {
      namespace field
      {
         namespace internal
         {
            const std::unordered_map<std::string,decltype(CASUAL_FIELD_NO_TYPE)>& name_to_type()
            {
               static const std::decay_t<decltype(name_to_type())> singleton
               {
                  {"short",   CASUAL_FIELD_SHORT},
                  {"long",    CASUAL_FIELD_LONG},
                  {"char",    CASUAL_FIELD_CHAR},
                  {"float",   CASUAL_FIELD_FLOAT},
                  {"double",  CASUAL_FIELD_DOUBLE},
                  {"string",  CASUAL_FIELD_STRING},
                  {"binary",  CASUAL_FIELD_BINARY},
               };

               return singleton;
            }

            const std::unordered_map<decltype(CASUAL_FIELD_NO_TYPE),std::string>& type_to_name()
            {
               static const std::decay_t<decltype(type_to_name())> singleton
               {
                  {CASUAL_FIELD_SHORT,    "short"},
                  {CASUAL_FIELD_LONG,     "long"},
                  {CASUAL_FIELD_CHAR,     "char"},
                  {CASUAL_FIELD_FLOAT,    "float"},
                  {CASUAL_FIELD_DOUBLE,   "double"},
                  {CASUAL_FIELD_STRING,   "string"},
                  {CASUAL_FIELD_BINARY,   "binary"},
               };

               return singleton;
            }

            namespace detail
            {

               namespace
               {

                  struct field
                  {
                     item_type id; // relative id
                     std::string name;
                     std::string type;
                     
                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        CASUAL_SERIALIZE( id);
                        CASUAL_SERIALIZE( name);
                        CASUAL_SERIALIZE( type);
                     })
                  };

                  struct group
                  {
                     item_type base = 0;
                     std::vector< field> fields;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        CASUAL_SERIALIZE( base);
                        CASUAL_SERIALIZE( fields);
                     })
                  };


                  struct mapping
                  {
                     item_type id;
                     std::string name;
                  };

                  std::vector<group> fetch_groups()
                  {
                     decltype(fetch_groups()) groups;

                     common::file::Input file{ common::environment::variable::get( "CASUAL_FIELD_TABLE")};
                     auto archive = common::serialize::create::reader::relaxed::from( file.extension(), file);
                     
                     archive >> CASUAL_NAMED_VALUE( groups);

                     return groups;
                  }

                  std::vector<field> fetch_fields()
                  {
                     const auto groups = fetch_groups();

                     decltype(fetch_fields()) fields;

                     for( const auto& group : groups)
                     {
                        for( const auto& field : group.fields)
                        {
                           try
                           {
                              const auto id = name_to_type().at( field.type) * CASUAL_FIELD_TYPE_BASE + group.base + field.id;

                              if( id > CASUAL_FIELD_NO_ID)
                              {
                                 fields.push_back( field);
                                 fields.back().id = id;
                              }
                              else
                              {
                                 // TODO: Much better
                                 log::line( log::category::warning, "id for ", field.name, " is invalid");
                              }
                           }
                           catch( const std::out_of_range&)
                           {
                              // TODO: Much better
                              log::line( log::category::warning, "type for ", field.name, " is invalid");
                           }
                        }
                     }

                     return fields;

                  }

                  std::unordered_map<std::string,item_type> name_to_id()
                  {
                     decltype( name_to_id()) result;

                     try
                     {
                        const auto fields = fetch_fields();

                        for( const auto& field : fields)
                        {
                           if( ! result.emplace( field.name, field.id).second)
                           {
                              // TODO: Much better
                              log::line( log::category::warning, "name for ", field.name, " is not unique");
                           }
                        }
                     }
                     catch( ...)
                     {
                        // TODO: Handle this in an other way ?
                        common::exception::handle();
                     }

                     return result;
                  }

                  std::unordered_map<item_type,std::string> id_to_name()
                  {
                     decltype( id_to_name()) result;

                     try
                     {
                        const auto fields = fetch_fields();

                        for( const auto& field : fields)
                        {
                           if( ! result.emplace( field.id, field.name).second)
                           {
                              // TODO: Much better
                              log::line( log::category::warning, "id for ", field.name, " is not unique");
                           }
                        }
                     }
                     catch( ...)
                     {
                        // TODO: Handle this in an other way ?
                        common::exception::handle();
                     }

                     return result;
                  }

               } // <unnamed>

            } // detail

            const std::unordered_map<std::string,item_type>& name_to_id()
            {
               static const auto singleton = detail::name_to_id();
               return singleton;
            }

            const std::unordered_map<item_type,std::string>& id_to_name()
            {
               static const auto singleton = detail::id_to_name();
               return singleton;
            }


            namespace
            {
               struct write
               {
                  const item_type id;
                  const char* occurrence;
                  write( const item_type id, const char* const occurrence) : id( id), occurrence( occurrence) {}

                  template<typename A>
                  void serialize( A& archive) const
                  {
                     const auto name = "name";

                     archive << common::serialize::named::value::make( id_to_name().at( id), name);

                     const auto value = "value";

                     using common::serialize::named::value::make;

                     const auto data = occurrence + data_offset;
                     const auto size = occurrence + size_offset;

                     switch( id / CASUAL_FIELD_TYPE_BASE)
                     {
                     case CASUAL_FIELD_SHORT:
                        archive << make( decode<short>( data), value);
                        break;
                     case CASUAL_FIELD_LONG:
                        archive << make( decode<long>( data), value);
                        break;
                     case CASUAL_FIELD_CHAR:
                        archive << make( *(data), value);
                        break;
                     case CASUAL_FIELD_FLOAT:
                        archive << make( decode<float>( data), value);
                        break;
                     case CASUAL_FIELD_DOUBLE:
                        archive << make( decode<double>( data), value);
                        break;
                     case CASUAL_FIELD_STRING:
                        archive << make( std::string( data), value);
                        break;
                     case CASUAL_FIELD_BINARY:
                     default:
                        archive << make( std::vector<char>( data, data + decode<size_type>( size)), value);
                        break;
                     }
                  }
               };


               struct read
               {

                  template<typename A>
                  void serialize( A& archive)
                  {
                     std::string name;
                     archive >> CASUAL_NAMED_VALUE( name);

                     casual_field_id_of_name( name.c_str(), &m_id);

                     switch( m_id / CASUAL_FIELD_TYPE_BASE)
                     {
                     case CASUAL_FIELD_SHORT:
                        assign< short>( archive);
                        break;
                     case CASUAL_FIELD_LONG:
                        assign< long>( archive);
                        break;
                     case CASUAL_FIELD_CHAR:
                        assign< char>( archive);
                        break;
                     case CASUAL_FIELD_FLOAT:
                        assign< float>( archive);
                        break;
                     case CASUAL_FIELD_DOUBLE:
                        assign< double>( archive);
                        break;
                     case CASUAL_FIELD_STRING:
                        assign_string( archive);
                        break;
                     case CASUAL_FIELD_BINARY:
                     default:
                        archive >> common::serialize::named::value::make( m_value, "value");
                        break;
                     }
                  }

                  template< typename F>
                  void dispatch( F&& functor) const
                  {
                     switch( m_id / CASUAL_FIELD_TYPE_BASE)
                     {
                     case CASUAL_FIELD_SHORT:
                        dispatch_value< short>( functor);
                        break;
                     case CASUAL_FIELD_LONG:
                        dispatch_value< long>( functor);
                        break;
                     case CASUAL_FIELD_CHAR:
                        dispatch_value< char>( functor);
                        break;
                     case CASUAL_FIELD_FLOAT:
                        dispatch_value< float>( functor);
                        break;
                     case CASUAL_FIELD_DOUBLE:
                        dispatch_value< double>( functor);
                        break;
                     case CASUAL_FIELD_STRING:
                        dispatch_string( functor);
                        break;
                     case CASUAL_FIELD_BINARY:
                     default:
                        functor( m_id, m_value);
                        break;
                     }
                  }

               private:

                  template< typename T, typename A>
                  void assign( A& archive)
                  {
                     m_value.resize( sizeof( T));
                     archive >> common::serialize::named::value::make( *reinterpret_cast< T*>( m_value.data()), "value");
                  }

                  template< typename A>
                  void assign_string( A& archive)
                  {
                     std::string value;
                     archive >> CASUAL_NAMED_VALUE( value);
                     common::algorithm::copy( value, m_value);
                  }

                  template< typename T, typename F>
                  void dispatch_value( F& functor) const
                  {
                     functor( m_id, *reinterpret_cast< const T*>( m_value.data()));
                  }

                  template< typename F>
                  void dispatch_string( F& functor) const
                  {
                     std::string value( std::begin( m_value), std::end( m_value));
                     functor( m_id, value);
                  }

                  long m_id = 0;
                  std::vector< char> m_value;
               };


               struct Dispatch
               {
                  ~Dispatch()
                  {
                     pool_type::pool.deallocate( m_buffer);
                  }

                  auto release()
                  {
                     return std::move( pool_type::pool.release( std::exchange( m_buffer, nullptr)).payload);
                  }

                  void operator() ( long id, char value) { casual_field_add_char( &m_buffer, id, value);}
                  void operator() ( long id, short value) { casual_field_add_short( &m_buffer, id, value);}
                  void operator() ( long id, long value) { casual_field_add_long( &m_buffer, id, value);}
                  void operator() ( long id, float value) { casual_field_add_float( &m_buffer, id, value);}
                  void operator() ( long id, double value) { casual_field_add_double( &m_buffer, id, value);}
                  void operator() ( long id, const std::string& value) { casual_field_add_string( &m_buffer, id, value.c_str());}
                  void operator() ( long id, const std::vector< char>& value) { casual_field_add_binary( &m_buffer, id, value.data(), value.size());}

               private:
                  char* m_buffer = pool_type::pool.allocate( common::buffer::type::combine( CASUAL_FIELD), 1024);
               };


            } // <unnamed>

            namespace payload
            {
               void stream( common::buffer::Payload payload, std::ostream& stream, const std::string& protocol)
               {
                  const Trace trace{ "field::internal::stream out"};

                  const auto& buffer = pool_type::pool.get( pool_type::pool.insert( std::move( payload)));

                  common::log::line( verbose::log, "buffer.payload.type: ", buffer.payload.type, " - protocol: ", protocol);

                  auto archive = common::serialize::create::writer::from( protocol, stream);

                  std::vector< write> fields;

                  for( const auto& field : buffer.index)
                  {
                     for( const auto& occurrence : field.second)
                     {
                        fields.emplace_back( field.first, buffer.handle() + occurrence);
                     }
                  }

                  archive << CASUAL_NAMED_VALUE( fields);
               }

               common::buffer::Payload stream( std::istream& stream, const std::string& protocol)
               {
                  Trace trace{ "field::internal::stream in"};

                  log::line( verbose::log, "protocol: ", protocol);

                  auto archive = common::serialize::create::reader::relaxed::from( protocol, stream);

                  std::vector< read> fields;
                  archive >> CASUAL_NAMED_VALUE( fields);
                  archive.validate();

                  Dispatch dispatch;

                  for( auto& f : fields)
                  {
                     f.dispatch( dispatch);
                  }

                  return dispatch.release();
               }
            } // payload

            void stream( const char* buffer, std::ostream& stream, const std::string& protocol)
            {
               payload::stream( pool_type::pool.get( buffer).payload, stream, protocol);
            }

            char* stream( std::istream& stream, const std::string& protocol)
            {
               try
               {
                  return pool_type::pool.insert( payload::stream( stream, protocol));
               }
               catch( ...)
               {
                  common::exception::handle();
               }
               return nullptr;
            }


            char* add( platform::binary::type buffer)
            {
               const Trace trace{ "field::internal::add"};

               return pool_type::pool.insert( common::buffer::Payload{ common::buffer::type::combine( CASUAL_FIELD), std::move( buffer)});
            }

         } // internal

      } // field

   } // buffer

} // casual

int casual_field_name_of_id( const long id, const char** name)
{
   try
   {
      const auto& result = casual::buffer::field::internal::id_to_name().at( id);

      if( name)
      {
         *name = result.data();
      }
   }
   catch( ...)
   {
      return casual::buffer::field::error::handle();
   }

   return CASUAL_FIELD_SUCCESS;
}

int casual_field_id_of_name( const char* const name, long* const id)
{
   if( name)
   {
      try
      {
         const auto& result = casual::buffer::field::internal::name_to_id().at( name);

         if( id)
         {
            *id = result;
         }
      }
      catch( ...)
      {
         return casual::buffer::field::error::handle();
      }

      return CASUAL_FIELD_SUCCESS;
   }
   else
   {
      return CASUAL_FIELD_INVALID_ARGUMENT;
   }

}

int casual_field_type_of_id( const long id, int* const type)
{

   const int result = id / CASUAL_FIELD_TYPE_BASE;

   switch( result)
   {
      case CASUAL_FIELD_SHORT:
      case CASUAL_FIELD_LONG:
      case CASUAL_FIELD_CHAR:
      case CASUAL_FIELD_FLOAT:
      case CASUAL_FIELD_DOUBLE:
      case CASUAL_FIELD_STRING:
      case CASUAL_FIELD_BINARY:
         break;
      default:
         casual::common::log::line( casual::buffer::verbose::log, "invalid argument - id: ", id);
         return CASUAL_FIELD_INVALID_ARGUMENT;
   }

   if( type) *type = result;

   return CASUAL_FIELD_SUCCESS;

}

int casual_field_name_of_type( const int type, const char** name)
{
   try
   {
      const auto& result = casual::buffer::field::internal::type_to_name().at( type);

      if( name)
      {
         *name = result.data();
      }

   }
   catch( ...)
   {
      return casual::buffer::field::error::handle();
   }

   return CASUAL_FIELD_SUCCESS;
}

int casual_field_type_of_name( const char* const name, int* const type)
{
   try
   {
      const auto result = casual::buffer::field::internal::name_to_type().at( name);

      if( type)
      {
         *type = result;
      }
   }
   catch( ...)
   {
      return casual::buffer::field::error::handle();
   }

   return CASUAL_FIELD_SUCCESS;
}

int casual_field_plain_type_host_size( const int type, long* const count)
{
   if( count)
   {
      switch( type)
      {
      case CASUAL_FIELD_SHORT:
         *count = sizeof( short);
         break;
      case CASUAL_FIELD_LONG:
         *count = sizeof( long);
         break;
      case CASUAL_FIELD_CHAR:
         *count = sizeof( char);
         break;
      case CASUAL_FIELD_FLOAT:
         *count = sizeof( float);
         break;
      case CASUAL_FIELD_DOUBLE:
         *count = sizeof( double);
         break;
      default:
         return CASUAL_FIELD_INVALID_ARGUMENT;
      }
   }
   else
   {
      return CASUAL_FIELD_INVALID_ARGUMENT;
   }

   return CASUAL_FIELD_SUCCESS;
}

int casual_field_minimum_need( long id, long* count)
{
   if( count)
   {

      const constexpr auto item_size = casual::common::network::byteorder::bytes<casual::buffer::field::item_type>();
      const constexpr auto size_size = casual::common::network::byteorder::bytes<casual::buffer::field::size_type>();

      switch( id / CASUAL_FIELD_TYPE_BASE)
      {
      case CASUAL_FIELD_SHORT:
         *count = item_size + size_size + casual::common::network::byteorder::bytes<short>();
         break;
      case CASUAL_FIELD_LONG:
         *count = item_size + size_size + casual::common::network::byteorder::bytes<long>();
         break;
      case CASUAL_FIELD_CHAR:
         *count = item_size + size_size + casual::common::network::byteorder::bytes<char>();
         break;
      case CASUAL_FIELD_FLOAT:
         *count = item_size + size_size + casual::common::network::byteorder::bytes<float>();
         break;
      case CASUAL_FIELD_DOUBLE:
         *count = item_size + size_size + casual::common::network::byteorder::bytes<double>();
         break;
      case CASUAL_FIELD_STRING:
         *count = item_size + size_size + 1; // a null-terminator is always added
         break;
      case CASUAL_FIELD_BINARY:
         *count = item_size + size_size + 0; // can be empty
         break;
      default:
         return CASUAL_FIELD_INVALID_ARGUMENT;
      }
   }
   else
   {
      return CASUAL_FIELD_INVALID_ARGUMENT;
   }

   return CASUAL_FIELD_SUCCESS;
}



int casual_field_remove_all( char* const buffer)
{
   return casual::buffer::field::cut::all( buffer);
}

int casual_field_remove_id( char* buffer, const long id)
{
   long occurrences{};

   if( const auto result = casual::buffer::field::explore::count( buffer, id, occurrences))
   {
      return result;
   }

   while( occurrences)
   {
      casual::buffer::field::cut::data( buffer, id, --occurrences);
   }

   return CASUAL_FIELD_SUCCESS;
}

int casual_field_remove_occurrence( char* const buffer, const long id, long index)
{
   return casual::buffer::field::cut::data( buffer, id, index);
}

int casual_field_next( const char* const buffer, long* const id, long* const index)
{
   if( ! id || ! index)
   {
      return CASUAL_FIELD_INVALID_ARGUMENT;
   }

   if( *id == CASUAL_FIELD_NO_ID)
   {
      return casual::buffer::field::iterate::first( buffer, *id, *index);
   }
   else
   {
      return casual::buffer::field::iterate::next( buffer, *id, *index);
   }

}

int casual_field_copy_buffer( char** const target, const char* const source)
{
   return casual::buffer::field::copy::buffer( target, source);
}

int casual_field_copy_memory( char** const target, const void* const source, const long count)
{
   return casual::buffer::field::copy::memory( target, source, count);
}



namespace casual
{
   namespace buffer
   {
      namespace field
      {
         namespace transform
         {

            namespace
            {

               int stream( const char* const handle, std::ostream& stream)
               {
                  //const trace trace( "field::transform::stream");

                  try
                  {
                     const auto& buffer = pool_type::pool.get( handle);

                     stream << std::fixed;

                     for( const auto& field : buffer.index)
                     {
                        const auto& occurrences = field.second;

                        for( decltype(occurrences.size()) idx = 0; idx < occurrences.size(); ++idx)
                        {
                           stream << internal::id_to_name().at( field.first);

                           stream << '[' << idx << ']' << " = ";

                           const auto offset = buffer.payload.memory.data() + occurrences[idx];

                           const auto data = offset + data_offset;

                           switch( field.first / CASUAL_FIELD_TYPE_BASE)
                           {
                           case CASUAL_FIELD_SHORT:
                              stream << decode<short>( data);
                              break;
                           case CASUAL_FIELD_LONG:
                              stream << decode<long>( data);
                              break;
                           case CASUAL_FIELD_CHAR:
                              stream << data;
                              break;
                           case CASUAL_FIELD_FLOAT:
                              stream << decode<float>( data);
                              break;
                           case CASUAL_FIELD_DOUBLE:
                              stream << decode<double>( data);
                              break;
                           case CASUAL_FIELD_STRING:
                              stream << data;
                              break;
                           case CASUAL_FIELD_BINARY:
                           default:
                              const auto size = offset + size_offset;
                              stream << common::transcode::base64::encode( data, data + decode<long>( size));
                              break;
                           }

                           stream << '\n';
                        }
                     }
                  }
                  catch( ...)
                  {
                     return error::handle();
                  }

                  return CASUAL_FIELD_SUCCESS;

               }

            } // <unnamed>

         } // transform

      } // field

   } // buffer

} // casual



int casual_field_print( const char* const buffer)
{
   // TODO: Perhaps flush STDOUT ?
   return casual::buffer::field::transform::stream( buffer, std::cout);
}

int casual_field_match( const char* const buffer, const char* const expression, int* const match)
{
   std::ostringstream s;

   if( const auto result = casual::buffer::field::transform::stream( buffer, s))
   {
      return result;
   }

   try
   {
      const std::regex x( expression);

      if( match)
      {
         *match = std::regex_search( s.str(), x);
      }
   }
   catch( const std::regex_error&)
   {
      return CASUAL_FIELD_INVALID_ARGUMENT;
   }

   return CASUAL_FIELD_SUCCESS;
}

int casual_field_make_expression( const char* expression, const void** regex)
{
   try
   {
      *regex = new std::regex( expression, std::regex::optimize);
   }
   catch( const std::regex_error&)
   {
      return CASUAL_FIELD_INVALID_ARGUMENT;
   }

   return CASUAL_FIELD_SUCCESS;
}

int casual_field_match_expression( const char* buffer, const void* regex, int* match)
{
   std::ostringstream s;

   if( const auto result = casual::buffer::field::transform::stream( buffer, s))
   {
      return result;
   }

   try
   {
      if( match)
      {
         *match = std::regex_search( s.str(), *reinterpret_cast<const std::regex*>(regex));
      }
   }
   catch( const std::regex_error&)
   {
      return CASUAL_FIELD_INVALID_ARGUMENT;
   }

   return CASUAL_FIELD_SUCCESS;
}

int casual_field_free_expression( const void* regex)
{
   delete reinterpret_cast<const std::regex*>(regex);
   return CASUAL_FIELD_SUCCESS;
}


int casual_field_to_string( char* target, long size, const char* key, const char* buffer)
{
   try
   {
      using stream_type = casual::buffer::internal::field::string::stream::Output;
      casual::buffer::internal::field::string::convert::to( key, buffer, stream_type{ stream_type::view_type{ target, size}});
   }
   catch( ...)
   {
      return casual::buffer::field::error::handle();
   }

   return CASUAL_FIELD_SUCCESS;
}

int casual_field_from_string( char** buffer, const char* key, const char* source, long size)
{
   try
   {
      using stream_type = casual::buffer::internal::field::string::stream::Input;
      casual::buffer::internal::field::string::convert::from( key, stream_type{ stream_type::view_type{ source, size}}, buffer);
   }
   catch( ...)
   {
      return casual::buffer::field::error::handle();
   }

   return CASUAL_FIELD_SUCCESS;

}
