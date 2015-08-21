//
// field.cpp
//
//  Created on: 3 nov 2013
//      Author: Kristone
//

#include "buffer/field.h"

#include "common/environment.h"
#include "common/exception.h"
#include "common/network/byteorder.h"
#include "common/buffer/pool.h"
#include "common/buffer/type.h"
#include "common/log.h"
#include "common/platform.h"
#include "common/internal/trace.h"

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

         /*
          * This implementation contain a C-interface that offers functionality
          * for a replacement to FML32 (non-standard compared to XATMI)
          *
          * The implementation is quite cumbersome since the user-handle is the
          * actual underlying buffer which is a std::vector<char>::data() so we
          * cannot just use append data which might imply reallocation since
          * this is a "stateless" interface and thus search from start is done
          * all the time
          *
          * The main idea is to keep the buffer "ready to go" without the need
          * for extra marshalling when transported and thus data is stored in
          * network byteorder etc from start
          *
          * The values layout is |id|size|data...|
          *
          * The id - and size type are network-long
          *
          * Some things that might be explained that perhaps is not so obvious
          * with FML32 where the type (FLD_SHORT/CASUAL_FIELD_SHORT etc) can be
          * deduced from the id, i.e. every id for 'short' must be between 0x0
          * and 0x1FFFFFF and every id for 'long' must be between 0x2000000 and
          * 0x3FFFFFF etc. In this implementation no validation of whether the
          * id exists in the repository-table occurs but just from the type and
          * we're, for simplicity, CASUAL_FIELD_SHORT has base 0x2000000 and
          * for now there's no proper error-handling while handling the table-
          * repository and that has to improve and many other things can be
          * improved as well ... Sean Parent would've cry if he saw this
          *
          * The repository-implementation is a bit comme ci comme ca and to
          * provide something like 'mkfldhdr32' the functionality has to be
          * accessible from there (or vice versa)
          */


         namespace
         {

            class Buffer : public common::buffer::Buffer
            {

            public:

               typedef common::platform::binary_type::size_type size_type;
               typedef common::platform::binary_type::const_pointer const_data_type;

               struct compare_first
               {
                  const long first;
                  bool operator()( const std::pair<long,long>& pair) const { return pair.first == first;}
               };

               struct update_second
               {
                  const long value;
                  void operator()( std::pair<long,long>& pair) const { pair.second += value;}
               };


               //enum : decltype(common::network::byteorder::bytes<long>())
               enum : long
               {
                  data_offset = common::network::byteorder::bytes<long>() * 2,
                  size_offset = common::network::byteorder::bytes<long>() * 1,
                  item_offset = common::network::byteorder::bytes<long>() * 0
               };

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


            private:

               // TODO: Perhaps move this to common/algorithm.h
               template<typename Iterator, typename Predicate, typename Index>
               static Iterator find_if_index( Iterator first, Iterator last, Predicate p, Index i)
               {
                  using Reference = typename std::iterator_traits<Iterator>::reference;
                  return std::find_if( first, last, [&]( Reference item) { return p( item) && !(i--); } );
               }

               template<typename T>
               static T decode( const_data_type where)
               {
                  using network_type = common::network::byteorder::type<T>;
                  const auto encoded = *reinterpret_cast< const network_type*>( where);
                  return common::network::byteorder::decode<T>( encoded);
               }

            public:

               template<typename... A>
               Buffer( A&&... arguments) : common::buffer::Buffer( std::forward<A>( arguments)...)
               {
                  update_index();
               }

               bool copy( const Buffer& other)
               {
                  if( this->reserved() < other.utilized())
                  {
                     return false;
                  }

                  this->payload.memory = other.payload.memory;
                  this->m_index = other.m_index;

                  return true;
               }

               bool copy( const void* const data, const std::size_t size)
               {
                  if( payload.memory.capacity() < size)
                  {
                     return false;
                  }

                  payload.memory.assign(
                     static_cast<const_data_type>(data),
                     static_cast<const_data_type>(data) + size);

                  update_index();

                  return true;
               }


               const std::vector< std::pair< long, long> >& index() const
               {
                  return m_index;
               }


               std::vector< std::pair< long, long> >::const_iterator index( const long id, const long occurrence) const
               {
                  return find_if_index( m_index.begin(), m_index.end(), compare_first{ id}, occurrence);
               }

               std::vector< std::pair< long, long> >::iterator index( const long id, const long occurrence)
               {
                  return find_if_index( m_index.begin(), m_index.end(), compare_first{ id}, occurrence);
               }

               //! @throw std::out_of_range when occurrence is not found
               size_type offset( const long id, const long occurrence) const
               {
                  return m_index.at( index( id, occurrence) - m_index.begin()).second;
               }

               const_data_type find( const long id, const long occurrence) const
               {
                  try
                  {
                     return payload.memory.data() + offset( id, occurrence);
                  }
                  catch( const std::out_of_range&)
                  {
                     return nullptr;
                  }
               }

               bool append( const long id, const_data_type value, const long count)
               {
                  const auto total = utilized() + data_offset + count;

                  if( total > reserved())
                  {
                     return false;
                  }

                  //
                  // Append current offset to index
                  //
                  m_index.emplace_back( id, utilized());

                  //
                  // Append the id to buffer
                  //
                  append( id);

                  //
                  // Append the size to buffer
                  //
                  append( count);

                  //
                  // Append the data to buffer
                  //
                  payload.memory.insert( payload.memory.end(), value, value + count);

                  return true;

               }

               bool append( const long id, const char* const value)
               {
                  const auto count = std::strlen( value) + 1;
                  return append( id, value, count);
               }

               template<typename T>
               bool append( const long id, const T value)
               {
                  const auto encoded = common::network::byteorder::encode( value);
                  return append( id, reinterpret_cast<const_data_type>(&encoded), sizeof( encoded));
               }

               bool select( const long id, const long occurrence, const_data_type& value, long& count) const
               {
                  if( const auto where = find( id, occurrence))
                  {
                     value = where + data_offset;

                     count = decode<long>( where + size_offset);
                  }
                  else
                  {
                     return false;
                  }

                  return true;
               }

               bool select( const long id, const long occurrence, const char*& value) const
               {
                  if( const auto where = find( id, occurrence))
                  {
                     value = where + data_offset;
                  }
                  else
                  {
                     return false;
                  }

                  return true;
               }


               template<typename T>
               bool select( const long id, const long occurrence, T& value) const
               {
                  if( const auto where = find( id, occurrence))
                  {
                     value = decode<T>( where + data_offset);
                  }
                  else
                  {
                     return false;
                  }

                  return true;
               }


               bool update( const long id, const long occurrence, const_data_type data, const long size)
               {
                  const auto offset = index( id, occurrence);

                  if( offset != m_index.end())
                  {
                     const auto count = decode<long>( payload.memory.data() + offset->second + size_offset);

                     //
                     // With equal sizes, stuff could be optimized ... but no
                     //

                     if( (utilized() - count + size) > reserved())
                     {
                        //
                        // Value won't fit without reallocation
                        //
                        return false;
                     }

                     //
                     // Write the new size
                     //

                     const auto encoded = common::network::byteorder::encode( size);
                     std::copy(
                        reinterpret_cast<const_data_type>( &encoded),
                        reinterpret_cast<const_data_type>( &encoded) + sizeof( encoded),
                        payload.memory.begin() + offset->second + size_offset);


                     //
                     // Erase the old and insert the new ... and yes, it
                     // could be done more efficient but the whole idea
                     // with this functionality is rather stupid
                     //

                     payload.memory.insert(
                        payload.memory.erase(
                           payload.memory.begin() + offset->second + data_offset,
                           payload.memory.begin() + offset->second + data_offset + count),
                        data, data + size);


                     //
                     // Update offsets beyond this one
                     //
                     std::for_each( offset + 1, m_index.end(), update_second{size - count});

                  }
                  else
                  {
                     return false;
                  }

                  return true;

               }

               bool update( const long id, const long occurrence, const char* const value)
               {
                  const auto count = std::strlen( value) + 1;
                  return update( id, occurrence, value, count);
               }


               template<typename T>
               bool update( const long id, const long occurrence, const T value)
               {
                  try
                  {
                     const auto where = payload.memory.begin() + offset( id, occurrence);
                     const auto encoded = common::network::byteorder::encode( value);

                     std::copy(
                        reinterpret_cast<const_data_type>( &encoded),
                        reinterpret_cast<const_data_type>( &encoded) + sizeof( encoded),
                        where + data_offset);
                  }
                  catch( const std::out_of_range&)
                  {
                     return false;
                  }

                  return true;
               }


               bool remove( const long id, const long occurrence)
               {
                  const auto offset = index( id, occurrence);

                  if( offset != m_index.end())
                  {
                     const auto count = decode<long>( payload.memory.data() + offset->second + size_offset);

                     //
                     // Remove the data from the buffer
                     //
                     payload.memory.erase(
                        payload.memory.begin() + offset->second,
                        payload.memory.begin() + offset->second + data_offset + count);

                     //
                     // Remove entry and update offsets
                     //
                     std::for_each(
                        m_index.erase( offset),
                        m_index.end(),
                        update_second{0 - data_offset - count});

                  }
                  else
                  {
                     return false;
                  }

                  return true;

               }

               void reset()
               {
                  payload.memory.clear();
                  m_index.clear();
               }


               long length( const long id, const long index, long& count)
               {
                  if( const auto where = find( id, index))
                  {
                     count = decode<long>( where + size_offset);
                  }
                  else
                  {
                     return false;
                  }

                  return true;
               }


               long count() const
               {
                  return m_index.size();
               }

               long count( const long id) const
               {
                  return std::count_if( m_index.begin(), m_index.end(), compare_first{ id});
               }

            private:

               template<typename T>
               void append( const T value)
               {
                  const auto encoded = common::network::byteorder::encode( value);
                  payload.memory.insert(
                     payload.memory.end(),
                     reinterpret_cast<const_data_type>( &encoded),
                     reinterpret_cast<const_data_type>( &encoded) + sizeof( encoded));
               }

               void update_index()
               {
                  m_index.clear();

                  const auto begin = payload.memory.begin();
                  const auto end = payload.memory.end();
                  auto cursor = begin;

                  while( cursor < end)
                  {
                     const auto id = decode<long>( &*cursor + item_offset);
                     const auto size = decode<long>( &*cursor + size_offset);

                     m_index.emplace_back( id, std::distance( begin, cursor));

                     std::advance( cursor, data_offset + size);
                  }
               }

            private:

               std::vector< std::pair< long, long> > m_index;

            };


            class Allocator : public common::buffer::pool::basic_pool<Buffer>
            {
            public:

               using types_type = common::buffer::pool::default_pool::types_type;

               static const types_type& types()
               {
                  // The types this pool can manage
                  static const types_type result{{ CASUAL_FIELD, "" }};
                  return result;
               }

               common::platform::raw_buffer_type allocate( const common::buffer::Type& type, const common::platform::binary_size_type size)
               {
                  m_pool.emplace_back( type, 0);

                  // GCC returns null for sad::vector::data with size zero
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

         } //

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

            struct trace : common::trace::internal::Scope
            {
               explicit trace( std::string information) : Scope( std::move( information), common::log::internal::buffer) {}
            };


            Buffer* find( const char* const handle)
            {
               //const trace trace( "field::find");

               try
               {
                  auto& buffer = pool_type::pool.get( handle);

                  return &buffer;
               }
               catch( ...)
               {
                  //
                  // TODO: Perhaps have some dedicated field-logging ?
                  //
                  common::error::handler();
               }

               return nullptr;
            }

            int remove( const char* const handle, const long id, long index)
            {
               //const trace trace( "field::remove");

               if( ! (id > CASUAL_FIELD_NO_ID))
               {
                  return CASUAL_FIELD_INVALID_ID;
               }

               if( const auto buffer = find( handle))
               {
                  if( buffer->remove( id, index))
                  {
                     return CASUAL_FIELD_SUCCESS;
                  }
                  else
                  {
                     return CASUAL_FIELD_NO_OCCURRENCE;
                  }
               }
               else
               {
                  return CASUAL_FIELD_INVALID_BUFFER;
               }


            }

            template<typename... A>
            int add( const char* const handle, const long id, const int type, A&&... arguments)
            {
               //const trace trace( "field::add");

               if( type != (id / CASUAL_FIELD_TYPE_BASE))
               {
                  return CASUAL_FIELD_INVALID_ID;
               }

               if( const auto buffer = find( handle))
               {
                  if( buffer->append( id, std::forward<A>( arguments)...))
                  {
                     return CASUAL_FIELD_SUCCESS;
                  }
                  else
                  {
                     return CASUAL_FIELD_NO_SPACE;
                  }
               }
               else
               {
                  return CASUAL_FIELD_INVALID_BUFFER;
               }

            }

            template<typename... A>
            int update( const char* const handle, const long id, long index, const int type, A&&... arguments)
            {
               //const trace trace( "field::update");

               if( type != (id / CASUAL_FIELD_TYPE_BASE))
               {
                  return CASUAL_FIELD_INVALID_ID;
               }

               if( const auto buffer = find( handle))
               {
                  if( buffer->update( id, index, std::forward<A>( arguments)...))
                  {
                     return CASUAL_FIELD_SUCCESS;
                  }

                  if( buffer->find( id, index))
                  {
                     return CASUAL_FIELD_NO_SPACE;;
                  }

                  return CASUAL_FIELD_NO_OCCURRENCE;

               }

               return CASUAL_FIELD_INVALID_BUFFER;

            }

            template<typename... A>
            int get( const char* const handle, const long id, long index, const int type, A&&... arguments)
            {
               //const trace trace( "field::get");

               if( type != (id / CASUAL_FIELD_TYPE_BASE))
               {
                  return CASUAL_FIELD_INVALID_ID;
               }

               if( const auto buffer = find( handle))
               {
                  if( buffer->select( id, index, std::forward<A>( arguments)...))
                  {
                     return CASUAL_FIELD_SUCCESS;
                  }

                  return CASUAL_FIELD_NO_OCCURRENCE;

               }

               return CASUAL_FIELD_INVALID_BUFFER;

            }

            int length( const char* const handle, const long id, const long index, long& count)
            {
               if( const auto buffer = find( handle))
               {
                  if( buffer->length( id, index, count))
                  {
                     return CASUAL_FIELD_SUCCESS;
                  }

                  return CASUAL_FIELD_NO_OCCURRENCE;

               }

               return CASUAL_FIELD_INVALID_BUFFER;

            }

            int exist( const char* const handle, const long id, const long index)
            {
               if( const auto buffer = find( handle))
               {
                  if( buffer->find( id, index))
                  {
                     return CASUAL_FIELD_SUCCESS;
                  }

                  return CASUAL_FIELD_NO_OCCURRENCE;

               }

               return CASUAL_FIELD_INVALID_BUFFER;

            }


            int next( const char* const handle, long& id, long& index)
            {
               //const trace trace( "field::next");

               if( const Buffer* const buffer = find( handle))
               {
                  const auto current = buffer->index( id, index);

                  if( current != buffer->index().end())
                  {
                     const auto adjacent = current + 1;

                     if( adjacent != buffer->index().end())
                     {
                        id = adjacent->first;
                        index = std::count_if( buffer->index().begin(), adjacent, Buffer::compare_first{ id});
                     }
                     else
                     {
                        return CASUAL_FIELD_NO_OCCURRENCE;
                     }
                  }
                  else
                  {
                     //
                     // We couldn't even find the previous one
                     //
                     return CASUAL_FIELD_INVALID_ID;
                  }

               }
               else
               {
                  return CASUAL_FIELD_INVALID_BUFFER;
               }

               return CASUAL_FIELD_SUCCESS;

            }

            int first( const char* const handle, long& id, long& index)
            {
               //const trace trace( "field::first");

               if( const auto buffer = find( handle))
               {
                  if( buffer->index().empty())
                  {
                     return CASUAL_FIELD_NO_OCCURRENCE;
                  }

                  id = buffer->index().at( 0).first;
                  index = 0;

               }
               else
               {
                  return CASUAL_FIELD_INVALID_BUFFER;
               }

               return CASUAL_FIELD_SUCCESS;

            }


            int count( const char* const handle, const long id, long& occurrences)
            {
               //const trace trace( "field::count");

               if( const auto buffer = find( handle))
               {
                  occurrences = buffer->count( id);
               }
               else
               {
                  return CASUAL_FIELD_INVALID_BUFFER;
               }

               return CASUAL_FIELD_SUCCESS;
            }

            int count( const char* const handle, long& occurrences)
            {
               //const trace trace( "field::count");

               if( const auto buffer = find( handle))
               {
                  occurrences = buffer->count();
               }
               else
               {
                  return CASUAL_FIELD_INVALID_BUFFER;
               }

               return CASUAL_FIELD_SUCCESS;
            }


            int reset( const char* const handle)
            {
               //const trace trace( "field::reset");

               if( const auto buffer = find( handle))
               {
                  buffer->reset();
               }
               else
               {
                  return CASUAL_FIELD_INVALID_BUFFER;
               }

               return CASUAL_FIELD_SUCCESS;

            }

            int explore( const char* const handle, long* const size, long* const used)
            {
               //const trace trace( "field::explore");

               if( const auto buffer = find( handle))
               {
                  if( size) *size = buffer->reserved();
                  if( used) *used = buffer->utilized();
               }
               else
               {
                  return CASUAL_FIELD_INVALID_BUFFER;
               }

               return CASUAL_FIELD_SUCCESS;
            }

            int copy( const char* const target_handle, const char* const source_handle)
            {
               //const trace trace( "field::copy");

               const auto target = find( target_handle);

               const auto source = find( source_handle);

               if( target && source)
               {
                  if( target->copy( *source))
                  {
                     return CASUAL_FIELD_SUCCESS;
                  }

                  return CASUAL_FIELD_NO_SPACE;

               }

               return CASUAL_FIELD_INVALID_BUFFER;

            }

            int serialize( char* const handle, const void* const source, const long count)
            {
               //const trace trace( "field::serialize");

               if( const auto buffer = casual::buffer::field::find( handle))
               {
                  if( buffer->copy( source, count))
                  {
                     return CASUAL_FIELD_SUCCESS;
                  }

                  return CASUAL_FIELD_NO_SPACE;

               }

               return CASUAL_FIELD_INVALID_BUFFER;

            }

         } //


      } // field

   } // buffer

} // casual


const char* CasualFieldDescription( const int code)
{
   switch( code)
   {
      case CASUAL_FIELD_SUCCESS:
         return "Success";
      case CASUAL_FIELD_NO_SPACE:
         return "No space";
      case CASUAL_FIELD_NO_OCCURRENCE:
         return "No occurrence";
      case CASUAL_FIELD_UNKNOWN_ID:
         return "Unknown id";
      case CASUAL_FIELD_INVALID_BUFFER:
         return "Invalid buffer";
      case CASUAL_FIELD_INVALID_ID:
         return "Invalid id";
      case CASUAL_FIELD_INVALID_TYPE:
         return "Invalid type";
      case CASUAL_FIELD_INVALID_ARGUMENT:
         return "Invalid argument";
      case CASUAL_FIELD_SYSTEM_FAILURE:
         return "System failure";
      case CASUAL_FIELD_INTERNAL_FAILURE:
         return "Internal failure";
      default:
         return "Unknown code";
   }
}

int CasualFieldExploreBuffer( const char* buffer, long* const size, long* const used)
{
   return casual::buffer::field::explore( buffer, size, used);
}

int CasualFieldExploreValue( const char* const buffer, const long id, const long index, long* const count)
{
   if( count)
   {
      const auto type = id / CASUAL_FIELD_TYPE_BASE;

      switch( type)
      {
      case CASUAL_FIELD_STRING:
      case CASUAL_FIELD_BINARY:
         return casual::buffer::field::length( buffer, id, index, *count);
      default:
         break;
      }

      if( const auto result = CasualFieldPlainTypeHostSize( type, count))
      {
         return result;
      }

   }

   return casual::buffer::field::exist( buffer, id, index);

}

int CasualFieldOccurrencesOfId( const char* const buffer, const long id, long* const occurrences)
{
   if( occurrences && id != CASUAL_FIELD_NO_ID)
   {
      return casual::buffer::field::count( buffer, id, *occurrences);
   }

   return CASUAL_FIELD_INVALID_ARGUMENT;
}

int CasualFieldOccurrencesInBuffer( const char* const buffer, long* const occurrences)
{
   if( occurrences)
   {
      return casual::buffer::field::count( buffer, *occurrences);
   }

   return CASUAL_FIELD_INVALID_ARGUMENT;
}



int CasualFieldAddChar( char* const buffer, const long id, const char value)
{
   return casual::buffer::field::add( buffer, id, CASUAL_FIELD_CHAR, value);
}

int CasualFieldAddShort( char* const buffer, const long id, const short value)
{
   return casual::buffer::field::add( buffer, id, CASUAL_FIELD_SHORT, value);
}

int CasualFieldAddLong( char* const buffer, const long id, const long value)
{
   return casual::buffer::field::add( buffer, id, CASUAL_FIELD_LONG, value);
}
int CasualFieldAddFloat( char* const buffer, const long id, const float value)
{
   return casual::buffer::field::add( buffer, id, CASUAL_FIELD_FLOAT, value);
}

int CasualFieldAddDouble( char* const buffer, const long id, const double value)
{
   return casual::buffer::field::add( buffer, id, CASUAL_FIELD_DOUBLE, value);
}

int CasualFieldAddString( char* const buffer, const long id, const char* const value)
{
   return casual::buffer::field::add( buffer, id, CASUAL_FIELD_STRING, value);
}

int CasualFieldAddBinary( char* const buffer, const long id, const char* const value, const long count)
{
   if( count < 0 )
   {
      return CASUAL_FIELD_INVALID_ARGUMENT;
   }

   return casual::buffer::field::add( buffer, id, CASUAL_FIELD_BINARY, value, count);
}

int CasualFieldAddValue( char* const buffer, const long id, const void* const value, const long count)
{

   switch( id / CASUAL_FIELD_TYPE_BASE)
   {
   case CASUAL_FIELD_SHORT:
      return CasualFieldAddShort ( buffer, id, *static_cast<const short*>( value));
   case CASUAL_FIELD_LONG:
      return CasualFieldAddLong  ( buffer, id, *static_cast<const long*>( value));
   case CASUAL_FIELD_CHAR:
      return CasualFieldAddChar  ( buffer, id, *static_cast<const char*>( value));
   case CASUAL_FIELD_FLOAT:
      return CasualFieldAddFloat ( buffer, id, *static_cast<const float*>( value));
   case CASUAL_FIELD_DOUBLE:
      return CasualFieldAddDouble( buffer, id, *static_cast<const double*>( value));
   case CASUAL_FIELD_STRING:
      return CasualFieldAddString( buffer, id, static_cast<const char*>( value));
   case CASUAL_FIELD_BINARY:
      return CasualFieldAddBinary( buffer, id, static_cast<const char*>( value), count);
   default:
      return CASUAL_FIELD_INVALID_ID;
   }

}

int CasualFieldAddEmpty( char* const buffer, const long id)
{
   switch( id / CASUAL_FIELD_TYPE_BASE)
   {
   case CASUAL_FIELD_SHORT:
      return CasualFieldAddShort ( buffer, id, 0);
   case CASUAL_FIELD_LONG:
      return CasualFieldAddLong  ( buffer, id, 0);
   case CASUAL_FIELD_CHAR:
      return CasualFieldAddChar  ( buffer, id, '\0');
   case CASUAL_FIELD_FLOAT:
      return CasualFieldAddFloat ( buffer, id, 0.0);
   case CASUAL_FIELD_DOUBLE:
      return CasualFieldAddDouble( buffer, id, 0.0);
   case CASUAL_FIELD_STRING:
      return CasualFieldAddString( buffer, id, "");
   case CASUAL_FIELD_BINARY:
      return CasualFieldAddBinary( buffer, id, "", 0);
   default:
      return CASUAL_FIELD_INVALID_ID;
   }

}


int CasualFieldGetChar( const char* const buffer, const long id, const long index, char* const value)
{
   return casual::buffer::field::get( buffer, id, index, CASUAL_FIELD_CHAR, *value);
}

int CasualFieldGetShort( const char* const buffer, const long id, const long index, short* const value)
{
   return casual::buffer::field::get( buffer, id, index, CASUAL_FIELD_SHORT, *value);
}

int CasualFieldGetLong( const char* const buffer, const long id, const long index, long* const value)
{
   return casual::buffer::field::get( buffer, id, index, CASUAL_FIELD_LONG, *value);
}

int CasualFieldGetFloat( const char* const buffer, const long id, const long index, float* const value)
{
   return casual::buffer::field::get( buffer, id, index, CASUAL_FIELD_FLOAT, *value);
}

int CasualFieldGetDouble( const char* const buffer, const long id, const long index, double* const value)
{
   return casual::buffer::field::get( buffer, id, index, CASUAL_FIELD_DOUBLE, *value);
}

int CasualFieldGetString( const char* const buffer, const long id, const long index, const char** value)
{
   return casual::buffer::field::get( buffer, id, index, CASUAL_FIELD_STRING, *value);
}

int CasualFieldGetBinary( const char* const buffer, const long id, const long index, const char** value, long* const count)
{
   return casual::buffer::field::get( buffer, id, index, CASUAL_FIELD_BINARY, *value, *count);
}

int CasualFieldGetValue( const char* const buffer, const long id, const long index, void* const value, long* const count)
{
   if( ! value)
   {
      return CasualFieldExploreValue( buffer, id, index, count);
   }

   const int type = id / CASUAL_FIELD_TYPE_BASE;

   const char* data = nullptr;
   long size = 0;

   switch( type)
   {
   case CASUAL_FIELD_STRING:
   case CASUAL_FIELD_BINARY:
      if( const auto result = casual::buffer::field::get( buffer, id, index, type, data, size))
         return result;
      break;
   default:
      if( const auto result = CasualFieldPlainTypeHostSize( type, &size))
         return result;
      break;
   }

   if( count)
   {
      if( *count < size)
      {
         return CASUAL_FIELD_NO_SPACE;
      }

      //
      // This is perhaps not Fget32-compatible if field is invalid
      //
      *count = size;
   }

   switch( type)
   {
   case CASUAL_FIELD_SHORT:
      return CasualFieldGetShort( buffer, id, index, static_cast<short*>( value));
   case CASUAL_FIELD_LONG:
      return CasualFieldGetLong( buffer, id, index, static_cast<long*>( value));
   case CASUAL_FIELD_CHAR:
      return CasualFieldGetChar( buffer, id, index, static_cast<char*>( value));
   case CASUAL_FIELD_FLOAT:
      return CasualFieldGetFloat( buffer, id, index, static_cast<float*>( value));
   case CASUAL_FIELD_DOUBLE:
      return CasualFieldGetDouble( buffer, id, index, static_cast<double*>( value));
   default:
      break;
   }

   std::memcpy( value, data, size);

   return CASUAL_FIELD_SUCCESS;

}

int CasualFieldUpdateChar( char* const buffer, const long id, const long index, const char value)
{
   return casual::buffer::field::update( buffer, id, index, CASUAL_FIELD_CHAR, value);
}

int CasualFieldUpdateShort( char* const buffer, const long id, const long index, const short value)
{
   return casual::buffer::field::update( buffer, id, index, CASUAL_FIELD_SHORT, value);
}

int CasualFieldUpdateLong( char* const buffer, const long id, const long index, const long value)
{
   return casual::buffer::field::update( buffer, id, index, CASUAL_FIELD_LONG, value);
}

int CasualFieldUpdateFloat( char* const buffer, const long id, const long index, const float value)
{
   return casual::buffer::field::update( buffer, id, index, CASUAL_FIELD_FLOAT, value);
}

int CasualFieldUpdateDouble( char* const buffer, const long id, const long index, const double value)
{
   return casual::buffer::field::update( buffer, id, index, CASUAL_FIELD_DOUBLE, value);
}

int CasualFieldUpdateString( char* const buffer, const long id, const long index, const char* const value)
{
   if( value)
   {
      return casual::buffer::field::update( buffer, id, index, CASUAL_FIELD_STRING, value);
   }

   return CASUAL_FIELD_INVALID_ARGUMENT;
}

int CasualFieldUpdateBinary( char* const buffer, const long id, const long index, const char* const value, const long count)
{
   if( count < 0)
   {
      return CASUAL_FIELD_INVALID_ARGUMENT;
   }

   return casual::buffer::field::update( buffer, id, index, CASUAL_FIELD_BINARY, value, count);
}

int CasualFieldUpdateValue( char* const buffer, const long id, const long index, const void* const value, const long count)
{
   if( ! value)
   {
      return CASUAL_FIELD_INVALID_ARGUMENT;
   }

   switch( id / CASUAL_FIELD_TYPE_BASE)
   {
   case CASUAL_FIELD_SHORT:
      return CasualFieldUpdateShort ( buffer, id, index, *static_cast<const short*>( value));
   case CASUAL_FIELD_LONG:
      return CasualFieldUpdateLong  ( buffer, id, index, *static_cast<const long*>( value));
   case CASUAL_FIELD_CHAR:
      return CasualFieldUpdateChar  ( buffer, id, index, *static_cast<const char*>( value));
   case CASUAL_FIELD_FLOAT:
      return CasualFieldUpdateFloat ( buffer, id, index, *static_cast<const float*>( value));
   case CASUAL_FIELD_DOUBLE:
      return CasualFieldUpdateDouble( buffer, id, index, *static_cast<const double*>( value));
   case CASUAL_FIELD_STRING:
      return CasualFieldUpdateString( buffer, id, index, static_cast<const char*>( value));
   case CASUAL_FIELD_BINARY:
      return CasualFieldUpdateBinary( buffer, id, index, static_cast<const char*>( value), count);
   default:
      return CASUAL_FIELD_INVALID_ID;
   }

}

int CasualFieldChangeValue( char* const buffer, const long id, const long index, const void* const value, const long count)
{
   if( index < 0)
   {
      return CasualFieldAddValue( buffer, id, value, count);
   }

   //
   // Can be more efficient but remember to add empty occurrences if needed
   //

   const auto result = CasualFieldUpdateValue( buffer, id, index, value, count);

   if( result == CASUAL_FIELD_NO_OCCURRENCE)
   {
      if( const auto result = CasualFieldAddEmpty( buffer, id))
      {
         return result;
      }

      return CasualFieldChangeValue( buffer, id, index, value, count);

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
                  long base;
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

int CasualFieldNameOfId( const long id, const char** name)
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
         return CASUAL_FIELD_UNKNOWN_ID;
      }
   }
   else
   {
      return CASUAL_FIELD_INVALID_ID;
   }

   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldIdOfName( const char* const name, long* const id)
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
         return CASUAL_FIELD_UNKNOWN_ID;
      }
   }
   else
   {
      return CASUAL_FIELD_INVALID_ARGUMENT;
   }

   return CASUAL_FIELD_SUCCESS;

}

int CasualFieldTypeOfId( const long id, int* const type)
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
         return CASUAL_FIELD_INVALID_ID;
   }

   if( type) *type = result;

   return CASUAL_FIELD_SUCCESS;

}

int CasualFieldNameOfType( const int type, const char** name)
{
   const auto result = casual::buffer::field::repository::type_to_name( type);

   if( result)
   {
      if( name) *name = result;
   }
   else
   {
      return CASUAL_FIELD_INVALID_TYPE;
   }

   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldTypeOfName( const char* const name, int* const type)
{
   if( name)
   {
      const auto result = casual::buffer::field::repository::name_to_type( name);

      if( result)
      {
         if( type) *type = result;
      }
      else
      {
         return CASUAL_FIELD_INVALID_TYPE;
      }
   }
   else
   {
      return CASUAL_FIELD_INVALID_ARGUMENT;
   }

   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldPlainTypeHostSize( const int type, long* const count)
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
         //return CASUAL_FIELD_INVALID_ARGUMENT;
         return CASUAL_FIELD_INVALID_TYPE;
      }
   }
   else
   {
      return CASUAL_FIELD_INVALID_ARGUMENT;
   }

   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldRemoveAll( char* const buffer)
{
   return casual::buffer::field::reset( buffer);
}

int CasualFieldRemoveId( char* buffer, const long id)
{
   long occurrences{};

   if( const auto result = casual::buffer::field::count( buffer, id, occurrences))
   {
      return result;
   }

   if( occurrences)
   {
      while( occurrences)
      {
         casual::buffer::field::remove( buffer, id, --occurrences);
      }
   }
   else
   {
      return CASUAL_FIELD_NO_OCCURRENCE;
   }

   return CASUAL_FIELD_SUCCESS;

}

int CasualFieldRemoveOccurrence( char* const buffer, const long id, long index)
{
   return casual::buffer::field::remove( buffer, id, index);
}

int CasualFieldNext( const char* const buffer, long* const id, long* const index)
{
   if( ! id || ! index)
   {
      return CASUAL_FIELD_INVALID_ARGUMENT;
   }

   if( *id == CASUAL_FIELD_NO_ID)
   {
      return casual::buffer::field::first( buffer, *id, *index);
   }
   else
   {
      return casual::buffer::field::next( buffer, *id, *index);
   }

}

int CasualFieldCopyBuffer( char* const target, const char* const source)
{
   return casual::buffer::field::copy( target, source);
}

int CasualFieldCopyMemory( char* const target, const void* const source, const long count)
{
   return casual::buffer::field::serialize( target, source, count);
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
                  const Buffer* const buffer = find( handle);

                  if( ! buffer)
                  {
                     return CASUAL_FIELD_INVALID_BUFFER;
                  }

                  stream << std::fixed;

                  std::unordered_map<long,long> occurrences;

                  for( const auto& field : buffer->index())
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

                     const auto data = buffer->payload.memory.data() + field.second + Buffer::data_offset;

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
                     // TODO: Handle string+binary in ... some way ... do we need escaping ?
                     case CASUAL_FIELD_STRING:
                     case CASUAL_FIELD_BINARY:
                     default:
                        stream << data;
                        break;
                     }

                     stream << '\n';

                  }

                  return CASUAL_FIELD_SUCCESS;

               }

            } //

         } // transform

      } // field

   } // buffer

} // casual



int CasualFieldPrint( const char* const buffer)
{
   // TODO: Perhaps flush STDOUT ?
   return casual::buffer::field::transform::stream( buffer, std::cout);
}

int CasualFieldMatch( const char* const buffer, const char* const expression, int* const match)
{
   std::ostringstream stream;

   if( const auto result = casual::buffer::field::transform::stream( buffer, stream))
   {
      return result;
   }

   try
   {
      const std::regex x( expression);

      if( match) *match = std::regex_match( stream.str(), x);
   }
   catch( const std::regex_error&)
   {
      return CASUAL_FIELD_INVALID_ARGUMENT;
   }
   catch( ...)
   {
      // TODO: Log or remove this
      return CASUAL_FIELD_INTERNAL_FAILURE;
   }

   return CASUAL_FIELD_SUCCESS;
}
