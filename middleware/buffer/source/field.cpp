//!
//! casual
//!

#include "buffer/field.h"

#include "common/environment.h"
#include "common/exception.h"
#include "common/network/byteorder.h"
#include "common/buffer/pool.h"
#include "common/buffer/type.h"
#include "common/log.h"
#include "common/platform.h"
#include "common/internal/trace.h"
#include "common/algorithm.h"


#include "sf/namevaluepair.h"
#include "sf/archive/maker.h"

#include <cstring>

#include <string>
#include <vector>
#include <unordered_map>

#include <algorithm>
#include <iterator>

#include <regex>

#include <iostream>
#include <sstream>

namespace casual
{
   namespace buffer
   {
      namespace field
      {

         namespace
         {

            typedef common::platform::binary_type::size_type size_type;
            typedef common::platform::binary_type::const_pointer const_data_type;
            typedef common::platform::binary_type::pointer data_type;


            struct compare_first
            {
               const long first;
               template<typename T>
               bool operator()( const T& pair) const { return pair.first == first;}
            };

            struct update_second
            {
               const long value;
               template<typename T>
               void operator()( T& pair) const { pair.second += value;}
            };


            // TODO: Perhaps move this to common/algorithm.h
            template<typename Iterator, typename Predicate, typename Index>
            Iterator find_if_index( Iterator first, Iterator last, Predicate p, Index i) noexcept
            {
               using Reference = typename std::iterator_traits<Iterator>::reference;
               return std::find_if( first, last, [&]( Reference item) { return p( item) && ! ( i--);});
            }

            template<typename Container, typename Predicate, typename Index>
            auto find_if_index( Container& c, Predicate p, Index i) noexcept -> decltype( c.begin())
            {
               return find_if_index( std::begin( c), std::end( c), p, i);
            }

            template<typename Container, typename Integer>
            auto find_index( Container& c, Integer id, Integer index) noexcept -> decltype( c.begin())
            {
               return find_if_index( c, compare_first{ id}, index);
            }


            template<typename T>
            T decode( const_data_type where) noexcept
            {
               using network_type = common::network::byteorder::type<T>;
               const auto encoded = *reinterpret_cast< const network_type*>( where);
               return common::network::byteorder::decode<T>( encoded);
            }

            enum : long
            {
               data_offset = common::network::byteorder::bytes<long>() * 2,
               size_offset = common::network::byteorder::bytes<long>() * 1,
               item_offset = common::network::byteorder::bytes<long>() * 0,
            };

            struct Buffer : common::buffer::Buffer
            {
               std::vector< std::pair< long, long> > index;

               template<typename... A>
               Buffer( A&&... arguments) : common::buffer::Buffer( std::forward<A>( arguments)...)
               {
                  const auto begin = payload.memory.begin();
                  const auto end = payload.memory.end();
                  auto cursor = begin;

                  while( cursor < end)
                  {
                     const auto id = decode<long>( &*cursor + item_offset);
                     const auto size = decode<long>( &*cursor + size_offset);

                     index.emplace_back( id, std::distance( begin, cursor));

                     std::advance( cursor, data_offset + size);
                  }
               }

               void shrink()
               {
                  return payload.memory.shrink_to_fit();
               }

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

               const_data_type handle() const noexcept
               {
                  return payload.memory.data();
               }

               data_type handle() noexcept
               {
                  return payload.memory.data();
               }

               //!
               //! Implement Buffer::transport
               //!
               size_type transport( const size_type user_size) const
               {
                  //
                  // Just ignore user-size all together
                  //

                  return utilized();
               }

               //!
               //! Implement Buffer::reserved
               //!
               size_type reserved() const
               {
                  return capacity();
               }

            };


            //
            // Might be named Pool as well
            //
            class Allocator : public common::buffer::pool::basic_pool<Buffer>
            {
            public:

               using types_type = common::buffer::pool::default_pool::types_type;

               static const types_type& types()
               {
                  //
                  // The types this pool can manage
                  //
                  static const types_type result{ common::buffer::type::combine( CASUAL_FIELD)};
                  return result;
               }

               common::platform::raw_buffer_type allocate( const std::string& type, const common::platform::binary_size_type size)
               {
                  m_pool.emplace_back( type, 0);

                  // GCC returns null for std::vector::data with capacity zero
                  m_pool.back().capacity( size ? size : 1);

                  return m_pool.back().handle();
               }

               common::platform::raw_buffer_type reallocate( const common::platform::const_raw_buffer_type handle, const common::platform::binary_size_type size)
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

                  return result->handle();
               }

            };

         } // <unnamed>

      } // field

   } // buffer

   //
   // Register and define the type that can be used to get the custom pool
   //
   template class common::buffer::pool::Registration< buffer::field::Allocator>;


   namespace buffer
   {
      namespace field
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
                     common::error::handler();
                     return CASUAL_FIELD_INTERNAL_FAILURE;
                  }
               }
            }

            namespace add
            {

               template<typename B>
               void append( B& buffer, const_data_type data, const long size)
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
               void append( B& buffer, const long id, const_data_type data, const long size)
               {
                  const auto used = buffer.utilized();

                  try
                  {
                     //
                     // Append id to buffer
                     //
                     append( buffer, id);

                     //
                     // Append size to buffer
                     //
                     append( buffer, size);

                     //
                     // Append data to buffer
                     //
                     append( buffer, data, size);

                     //
                     // Append current offset to index
                     //
                     buffer.index.emplace_back( id, used);

                  }
                  catch( ...)
                  {
                     //
                     // Make sure to reset the size in case of exception
                     //
                     buffer.utilized( used);
                     throw;
                  }
               }

               template<typename B>
               void append( B& buffer, const long id, const char* const value)
               {
                  append( buffer, id, value, std::strlen( value) + 1);
               }

               template<typename B, typename T>
               void append( B& buffer, const long id, const T value)
               {
                  const auto encoded = common::network::byteorder::encode( value);
                  append( buffer, id, reinterpret_cast<const_data_type>( &encoded), sizeof( encoded));
               }

               template<typename... A>
               int data( char** handle, const long id, const int type, A&&... arguments)
               {
                  //const trace trace( "field::add::data");

                  if( type != (id / CASUAL_FIELD_TYPE_BASE))
                  {
                     return CASUAL_FIELD_INVALID_ARGUMENT;
                  }

                  try
                  {
                     auto& buffer = pool_type::pool.get( *handle);

                     //
                     // Make sure to update the handle regardless
                     //
                     const auto synchronize = common::scope::execute
                     ( [&]() { *handle = buffer.handle();});

                     //
                     // Append the data
                     //
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
               size_type offset( const I& index, const long id, const long occurrence)
               {
                  const auto iterator = find_index( index, id, occurrence);

                  return index.at( std::distance( index.begin(), iterator)).second;
               }

               template<typename B>
               void select( const B& buffer, const long id, const long occurrence, const_data_type& data, long& size)
               {
                  const auto field_offset = offset( buffer.index, id, occurrence);

                  data = buffer.handle() + field_offset + data_offset;

                  size = decode<long>( buffer.handle() + field_offset + size_offset);
               }

               template<typename B>
               void select( const B& buffer, const long id, const long occurrence, const_data_type& data)
               {
                  const auto field_offset = offset( buffer.index, id, occurrence);

                  data = buffer.handle() + field_offset + data_offset;
               }

               template<typename B, typename T>
               void select( const B& buffer, const long id, const long occurrence, T& data)
               {
                  const auto field_offset = offset( buffer.index, id, occurrence);

                  data = decode<T>( buffer.handle() + field_offset + data_offset);
               }


               template<typename... A>
               int data( const char* const handle, const long id, long occurrence, const int type, A&&... arguments)
               {
                  //const trace trace( "field::get::data");

                  if( type != (id / CASUAL_FIELD_TYPE_BASE))
                  {
                     return CASUAL_FIELD_INVALID_ARGUMENT;
                  }

                  try
                  {
                     const auto& buffer = pool_type::pool.get( handle);

                     //
                     // Select the data
                     //
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
               void remove( B& buffer, const long id, const long occurrence)
               {
                  const auto iterator = find_index( buffer.index, id, occurrence);

                  const auto field_offset = buffer.index.at( std::distance( buffer.index.begin(), iterator)).second;

                  //
                  // Get the size of the value
                  //
                  const auto size = decode<long>( buffer.handle() + field_offset + size_offset);

                  //
                  // Remove the data from the buffer
                  //
                  buffer.payload.memory.erase(
                     buffer.payload.memory.begin() + field_offset,
                     buffer.payload.memory.begin() + field_offset + data_offset + size);

                  //
                  // Remove entry and update offsets
                  //
                  std::for_each(
                     buffer.index.erase( iterator),
                     buffer.index.end(),
                     update_second{0 - data_offset - size});

               }


               int data( const char* const handle, const long id, long occurrence)
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
               void update( M& memory, I& index, const long id, const long occurrence, const_data_type data, const long size)
               {
                  const auto iterator = find_index( index, id, occurrence);

                  //
                  // Get the offset to where the field begin
                  //
                  const auto field_offset = index.at( std::distance( index.begin(), iterator)).second;

                  const auto current_size = decode<long>( memory.data() + field_offset + size_offset);


                  //
                  // With equal sizes, stuff could be optimized ... but no
                  //

                  //
                  // Erase the old and insert the new ... and yes, it
                  // could be done more efficient but the whole idea
                  // with this functionality is rather stupid
                  //

                  memory.insert(
                     memory.erase(
                        memory.begin() + field_offset + data_offset,
                        memory.begin() + field_offset + data_offset + current_size),
                     data, data + size);

                  //
                  // Write the new size (afterwards since above can throw)
                  //

                  const auto encoded = common::network::byteorder::encode( size);
                  std::copy(
                     reinterpret_cast<const_data_type>( &encoded),
                     reinterpret_cast<const_data_type>( &encoded) + sizeof( encoded),
                     memory.begin() + field_offset + size_offset);

                  //
                  // Update offsets beyond this one
                  //

                  std::for_each( iterator + 1, index.end(), update_second{size - current_size});

               }

               template<typename M, typename I>
               void update( M& memory, I& index, const long id, const long occurrence, const_data_type value)
               {
                  const auto count = std::strlen( value) + 1;
                  update( memory, index, id, occurrence, value, count);
               }


               template<typename M, typename I, typename T>
               void update( M& memory, I& index, const long id, const long occurrence, const T value)
               {
                  //
                  // Could be optimized, but ... no
                  //
                  const auto encoded = common::network::byteorder::encode( value);
                  update( memory, index, id, occurrence, reinterpret_cast<const_data_type>( &encoded), sizeof( encoded));
               }



               template<typename... A>
               int data( char** handle, const long id, long occurrence, const int type, A&&... arguments)
               {
                  //const trace trace( "field::set::data");

                  if( type != (id / CASUAL_FIELD_TYPE_BASE))
                  {
                     return CASUAL_FIELD_INVALID_ARGUMENT;
                  }

                  try
                  {
                     auto& buffer = pool_type::pool.get( *handle);

                     //
                     // Make sure to update the handle regardless
                     //
                     const auto synchronize = common::scope::execute
                     ( [&]() { *handle = buffer.handle();});

                     //
                     // Update the data
                     //
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
               int value( const char* const handle, const long id, const long occurrence, long& count)
               {
                  //const trace trace( "field::explore::value");

                  try
                  {
                     const auto& buffer = pool_type::pool.get( handle);

                     const auto iterator = find_index( buffer.index, id, occurrence);

                     const auto offset = buffer.index.at( std::distance( buffer.index.begin(), iterator)).second;

                     count = decode<long>( buffer.payload.memory.data() + offset + size_offset);
                  }
                  catch( ...)
                  {
                     return error::handle();
                  }

                  return CASUAL_FIELD_SUCCESS;
               }

               int buffer( const char* const handle, long& size, long& used)
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

               int existence( const char* const handle, const long id, const long occurrence)
               {
                  //const trace trace( "field::explore::existence");

                  try
                  {
                     const auto& buffer = pool_type::pool.get( handle);

                     if( find_index( buffer.index, id, occurrence) != buffer.index.end())
                        return CASUAL_FIELD_SUCCESS;
                     else
                        return CASUAL_FIELD_OUT_OF_BOUNDS;
                  }
                  catch( ...)
                  {
                     return error::handle();
                  }
               }


               int count( const char* const handle, const long id, long& occurrences)
               {
                  //const trace trace( "field::explore::count");

                  try
                  {
                     const auto& buffer = pool_type::pool.get( handle);

                     occurrences =
                        std::count_if(
                           buffer.index.begin(),
                           buffer.index.end(),
                           compare_first{ id});
                  }
                  catch( ...)
                  {
                     return error::handle();
                  }

                  return CASUAL_FIELD_SUCCESS;
               }

               int count( const char* const handle, long& occurrences)
               {
                  //const trace trace( "field::explore::count");

                  try
                  {
                     const auto& buffer = pool_type::pool.get( handle);

                     occurrences = buffer.index.size();
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
               int first( const char* const handle, long& id, long& index)
               {
                  //const trace trace( "field::iterate::first");

                  try
                  {
                     const auto& buffer = pool_type::pool.get( handle);
                     id = buffer.index.at( 0).first;
                     index = 0;
                  }
                  catch( ...)
                  {
                     return error::handle();
                  }

                  return CASUAL_FIELD_SUCCESS;

               }

               int next( const char* const handle, long& id, long& index)
               {
                  //const trace trace( "field::iterate::next");

                  try
                  {
                     const auto& buffer = pool_type::pool.get( handle);

                     const auto current = find_index( buffer.index, id, index);

                     if( current != buffer.index.end())
                     {
                        const auto adjacent = current + 1;

                        if( adjacent != buffer.index.end())
                        {
                           id = adjacent->first;
                           index = std::count_if( buffer.index.begin(), adjacent, compare_first{ id});
                        }
                        else
                        {
                           return CASUAL_FIELD_OUT_OF_BOUNDS;
                        }
                     }
                     else
                     {
                        //
                        // We couldn't even find the previous one
                        //
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

                     const auto synchronize = common::scope::execute
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

               int memory( char** const handle, const void* const source, const long count)
               {
                  //const trace trace( "field::copy::data");

                  try
                  {
                     auto& buffer = pool_type::pool.get( *handle);

                     const auto synchronize = common::scope::execute
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

            }

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
         //return CASUAL_FIELD_OUT_OF_MEMORY;
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
         namespace repository
         {
            std::unordered_map<std::string,int> name_to_type()
            {
               return decltype(name_to_type())
               {
                  {"short",   CASUAL_FIELD_SHORT},
                  {"long",    CASUAL_FIELD_LONG},
                  {"char",    CASUAL_FIELD_CHAR},
                  {"float",   CASUAL_FIELD_FLOAT},
                  {"double",  CASUAL_FIELD_DOUBLE},
                  {"string",  CASUAL_FIELD_STRING},
                  {"binary",  CASUAL_FIELD_BINARY},
               };
            }

            int name_to_type( const char* const name)
            {
               static const auto mapping = name_to_type();

               const auto type = mapping.find( name);

               return type != mapping.end() ? type->second : CASUAL_FIELD_NO_TYPE;
            }

            std::unordered_map<int,std::string> type_to_name()
            {
               return decltype(type_to_name())
               {
                  {CASUAL_FIELD_SHORT,    "short"},
                  {CASUAL_FIELD_LONG,     "long"},
                  {CASUAL_FIELD_CHAR,     "char"},
                  {CASUAL_FIELD_FLOAT,    "float"},
                  {CASUAL_FIELD_DOUBLE,   "double"},
                  {CASUAL_FIELD_STRING,   "string"},
                  {CASUAL_FIELD_BINARY,   "binary"},
               };
            }



            const char* type_to_name( const int type)
            {
               static const auto mapping = type_to_name();

               const auto name = mapping.find( type);

               return name != mapping.end() ? name->second.c_str() : nullptr;
            }

            namespace
            {

               struct field
               {
                  long id; // relative id
                  std::string name;
                  std::string type;
                  //std::string comment;

                  template< typename A>
                  void serialize( A& archive)
                  {
                     archive & CASUAL_MAKE_NVP( id);
                     archive & CASUAL_MAKE_NVP( name);
                     archive & CASUAL_MAKE_NVP( type);
                     //archive & CASUAL_MAKE_NVP( comment);
                  }
               };

               struct group
               {
                  long base = 0;
                  std::vector< field> fields;

                  template< typename A>
                  void serialize( A& archive)
                  {
                     archive & CASUAL_MAKE_NVP( base);
                     archive & CASUAL_MAKE_NVP( fields);
                  }
               };

               struct mapping
               {
                  long id;
                  std::string name;
               };

               std::vector<group> fetch_groups()
               {
                  decltype(fetch_groups()) groups;

                  const auto file = common::environment::variable::get( "CASUAL_FIELD_TABLE");

                  auto archive = sf::archive::reader::from::file( file);
                  archive >> CASUAL_MAKE_NVP( groups);

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
                              common::log::warning << "id for " << field.name << " is invalid" << std::endl;
                           }
                        }
                        catch( const std::out_of_range&)
                        {
                           // TODO: Much better
                           common::log::warning << "type for " << field.name << " is invalid" << std::endl;
                        }
                     }
                  }

                  return fields;

               }

            } //

            std::unordered_map<std::string,long> name_to_id()
            {
               const auto fields = fetch_fields();

               decltype( name_to_id()) result;

               for( const auto& field : fields)
               {
                  if( ! result.emplace( field.name, field.id).second)
                  {
                     // TODO: Much better
                     common::log::warning << "name for " << field.name << " is not unique" << std::endl;
                  }
               }

               return result;
            }

            std::unordered_map<long,std::string> id_to_name()
            {
               const auto fields = fetch_fields();

               decltype( id_to_name()) result;

               for( const auto& field : fields)
               {
                  if( ! result.emplace( field.id, field.name).second)
                  {
                     // TODO: Much better
                     common::log::warning << "id for " << field.name << " is not unique" << std::endl;
                  }
               }

               return result;

            }


            long name_to_id( const char* const name)
            {

               try
               {
                  static const auto mapping = name_to_id();

                  const auto id = mapping.find( name);

                  if( id != mapping.end())
                  {
                     return id->second;
                  }

               }
               catch( ...)
               {
                  // TODO: Handle this in an other way ?
                  casual::common::error::handler();
               }

               return CASUAL_FIELD_NO_ID;

            }

            const char* id_to_name( const long id)
            {
               try
               {
                  static const auto mapping = id_to_name();

                  const auto name = mapping.find( id);

                  if( name != mapping.end())
                  {
                     return name->second.c_str();
                  }

               }
               catch( ...)
               {
                  // TODO: Handle this in an other way ?
                  casual::common::error::handler();
               }

               return nullptr;

            }



         } // repository

      } // field

   } // buffer

} // casual

int casual_field_name_of_id( const long id, const char** name)
{
   if( id > CASUAL_FIELD_NO_ID)
   {
      const auto result = casual::buffer::field::repository::id_to_name( id);

      if( result)
      {
         if( name) *name = result;
      }
      else
      {
         return CASUAL_FIELD_OUT_OF_BOUNDS;
      }
   }
   else
   {
      return CASUAL_FIELD_INVALID_ARGUMENT;
   }

   return CASUAL_FIELD_SUCCESS;
}

int casual_field_id_of_name( const char* const name, long* const id)
{
   if( name)
   {
      const auto result = casual::buffer::field::repository::name_to_id( name);

      if( result)
      {
         if( id) *id = result;
      }
      else
      {
         return CASUAL_FIELD_OUT_OF_BOUNDS;
      }
   }
   else
   {
      return CASUAL_FIELD_INVALID_ARGUMENT;
   }

   return CASUAL_FIELD_SUCCESS;

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
         return CASUAL_FIELD_INVALID_ARGUMENT;
   }

   if( type) *type = result;

   return CASUAL_FIELD_SUCCESS;

}

int casual_field_name_of_type( const int type, const char** name)
{
   const auto result = casual::buffer::field::repository::type_to_name( type);

   if( result)
   {
      if( name) *name = result;

      return CASUAL_FIELD_SUCCESS;
   }

   return CASUAL_FIELD_INVALID_ARGUMENT;
}

int casual_field_type_of_name( const char* const name, int* const type)
{
   if( name)
   {
      const auto result = casual::buffer::field::repository::name_to_type( name);

      if( result)
      {
         if( type) *type = result;

         return CASUAL_FIELD_SUCCESS;
      }
   }

   return CASUAL_FIELD_INVALID_ARGUMENT;
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

   if( occurrences)
   {
      while( occurrences)
      {
         casual::buffer::field::cut::data( buffer, id, --occurrences);
      }
   }
   else
   {
      return CASUAL_FIELD_OUT_OF_BOUNDS;
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

               template<typename T>
               T pod( const char* const data)
               {
                  const auto encoded = *reinterpret_cast< const common::network::byteorder::type<T>*>( data);
                  return common::network::byteorder::decode<T>( encoded);
               }


               int stream( const char* const handle, std::ostream& stream)
               {
                  //const trace trace( "field::transform::stream");

                  try
                  {
                     const auto& buffer = pool_type::pool.get( handle);

                     stream << std::fixed;

                     std::unordered_map<long,long> occurrences;

                     for( const auto& field : buffer.index)
                     {
                        if( const auto name = repository::id_to_name( field.first))
                        {
                           stream << name;
                        }
                        else
                        {
                           stream << field.first;
                        }

                        stream << '[' << occurrences[field.first]++ << ']' << " = ";

                        const auto data = buffer.payload.memory.data() + field.second + data_offset;

                        switch( field.first / CASUAL_FIELD_TYPE_BASE)
                        {
                        case CASUAL_FIELD_SHORT:
                           stream << pod<short>( data);
                           break;
                        case CASUAL_FIELD_LONG:
                           stream << pod<long>( data);
                           break;
                        case CASUAL_FIELD_CHAR:
                           stream << *data;
                           break;
                        case CASUAL_FIELD_FLOAT:
                           stream << pod<float>( data);
                           break;
                        case CASUAL_FIELD_DOUBLE:
                           stream << pod<double>( data);
                           break;
                        case CASUAL_FIELD_STRING:
                        case CASUAL_FIELD_BINARY:
                        default:
                           //
                           // TODO: Handle string+binary in ... some way ... do we need escaping ?
                           //
                           stream << data;
                           break;
                        }

                        stream << '\n';

                     }

                  }
                  catch( ...)
                  {
                     return error::handle();
                  }

                  return CASUAL_FIELD_SUCCESS;

               }

            }

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
