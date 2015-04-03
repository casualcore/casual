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

#include "sf/namevaluepair.h"
#include "sf/archive/maker.h"

#include <cstring>

#include <string>
#include <map>
#include <vector>

#include <algorithm>

#include <type_traits>

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
          * repository and that has to improve ...
          *
          * ... and many other things can be improved as well. There's a lot of
          * semi-redundant functions for finding a certain occurrence which is
          * made by searching from start every time since we do not have any
          * index or such but some (perhaps naive) benchmarks shows that that
          * doesn't give isn't so slow after all ... future will show
          *
          * Removal of occurrences doesn't actually remove them but just make
          * them invisible by nullify it and thus the buffer doesn't collapse
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

               using common::buffer::Buffer::Buffer;

               typedef common::platform::binary_type::size_type size_type;

               //
               // Shall return the actual size needed
               //
               size_type size( const size_type user_size) const
               {
                  //
                  // Ignore user provided size utilized size
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

               typedef common::platform::raw_buffer_type data_type;

               data_type handle()
               {
                  return payload.memory.data();
               }

               data_type begin()
               {
                  return handle();
               }

               data_type end()
               {
                  return handle() + utilized();
               }

            private:

               size_type m_inserter = 0;

            };

            //
            // Helper
            //
            struct Value
            {
               typedef common::platform::raw_buffer_type data_type;

               static constexpr auto offset = common::network::byteorder::bytes<long>();

               data_type where = nullptr;

               Value( data_type where) : where( where) {}

               bool operator == ( const Value& other) const
               {
                  return this->where == other.where;
               }

               bool operator != ( const Value& other) const
               {
                  return this->where != other.where;
               }


               static constexpr auto header() -> decltype( offset)
               {
                  return offset * 2;
               }

               long id() const
               {
                  const auto encoded = *reinterpret_cast< const common::network::byteorder::type<long>*>( where + offset * 0);
                  return common::network::byteorder::decode<long>( encoded);
               }

               void id( const long value)
               {
                  const auto encoded = common::network::byteorder::encode( value);
                  std::memcpy( where + offset * 0, &encoded, sizeof( encoded));
               }

               long size() const
               {
                  const auto encoded = *reinterpret_cast< const common::network::byteorder::type<long>*>( where + offset * 1);
                  return common::network::byteorder::decode<long>( encoded);
               }

               void size( const long value)
               {
                  const auto encoded = common::network::byteorder::encode( value);
                  std::memcpy( where + offset * 1, &encoded, sizeof( encoded));
               }

               data_type data() const
               {
                  return where + offset * 2;
               }

               void data( const void* value, const std::size_t count)
               {
                  std::memcpy( data(), value, count);
               }

               //! @return A 'handle' to next 'iterator'
               Value next() const
               {
                  return Value( data() + size());
               }

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
                  //
                  // This should not need to be implemented, but 'cause of some
                  // probably non standard conformant behavior in GCC where
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

            Buffer* find_buffer( const char* const handle)
            {
               try
               {
                  auto& buffer = pool_type::pool.get( handle);

                  if( buffer.payload.type.name != CASUAL_FIELD)
                  {
                     //
                     // TODO: The pool can only be invoked with the registered type, so this
                     //   check is not needed...
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
                  // TODO: Perhaps have some dedicated field-logging ?
                  //
                  common::error::handler();
               }

               return nullptr;
            }

            int remove( const char* const handle, const long id)
            {
               if( ! (id > CASUAL_FIELD_NO_ID))
               {
                  return CASUAL_FIELD_INVALID_ID;
               }

               auto buffer = find_buffer( handle);

               if( ! buffer)
               {
                  return CASUAL_FIELD_INVALID_BUFFER;
               }

               int result = CASUAL_FIELD_NO_OCCURRENCE;

               const Value beyond( buffer->end());
               Value value( buffer->begin());

               while( value != beyond)
               {
                  if( value.id() == id)
                  {
                     value.id( CASUAL_FIELD_NO_ID);
                     // We found at least one
                     result = CASUAL_FIELD_SUCCESS;
                  }

                  value = value.next();
               }

               return result;

            }


            int remove( const char* const handle, const long id, long index)
            {
               if( ! (id > CASUAL_FIELD_NO_ID))
               {
                  return CASUAL_FIELD_INVALID_ID;
               }

               auto buffer = find_buffer( handle);

               if( !buffer)
               {
                  return CASUAL_FIELD_INVALID_BUFFER;
               }

               const Value beyond( buffer->end());
               Value value( buffer->begin());

               while( value != beyond)
               {
                  if( value.id() == id)
                  {
                     if( !index--)
                     {
                        value.id( CASUAL_FIELD_NO_ID);
                        return CASUAL_FIELD_SUCCESS;
                     }

                  }

                  value = value.next();

               }

               return CASUAL_FIELD_NO_OCCURRENCE;

            }


            int add( const char* const handle, const long id, const int type, const char* const data, const long size)
            {
               if( type != (id / CASUAL_FIELD_TYPE_BASE))
               {
                  return CASUAL_FIELD_INVALID_ID;
               }

               auto buffer = find_buffer( handle);

               if( ! buffer)
               {
                  return CASUAL_FIELD_INVALID_BUFFER;
               }

               if( ! data)
               {
                  return CASUAL_FIELD_INVALID_ARGUMENT;
               }


               const auto total = Value::header() + size + buffer->utilized();

               if( total > buffer->reserved())
               {
                  return CASUAL_FIELD_NO_SPACE;
               }

               Value value( buffer->end());

               value.id( id);
               value.size( size);
               value.data( data, size);
               buffer->utilized( total);

               return CASUAL_FIELD_SUCCESS;
            }

            template<typename T>
            int add( const char* const handle, const long id, const int type, const T data)
            {
               const auto encoded = common::network::byteorder::encode( data);
               return add( handle, id, type, reinterpret_cast<const char*>( &encoded), sizeof (encoded));
            }

            int update( const char* const handle, const long id, long index, const int type, const char* const data, const long size)
            {
               if( type != (id / CASUAL_FIELD_TYPE_BASE))
               {
                  return CASUAL_FIELD_INVALID_ID;
               }

               auto buffer = find_buffer( handle);

               if( ! buffer)
               {
                  return CASUAL_FIELD_INVALID_BUFFER;
               }

               const Value beyond( buffer->end());
               Value value( buffer->begin());

               while( value != beyond)
               {
                  if( value.id() == id)
                  {
                     if( ! index--)
                     {
                        //
                        // Found present value
                        //

                        const auto present = value.size();

                        if( present != size)
                        {
                           //
                           // Calculate new end
                           //

                           const auto required = buffer->utilized() - present + size;

                           if( required > buffer->reserved())
                           {
                              return CASUAL_FIELD_NO_SPACE;
                           }

                           //
                           // We need to move all following values
                           //

                           auto target = value.data() + size;
                           auto source = value.data() + present;

                           //
                           // Calculate how many bytes rest of the values represent
                           //
                           const auto count = 0 + buffer->end() - source;


                           //
                           // Move the data
                           //
                           std::memmove( target, source, count);

                           //
                           // Update the value with the new size
                           //
                           value.size( size);

                           //
                           // Update the buffer with the new total usage
                           //
                           buffer->utilized( required);

                        }


                        //
                        // Write the new data
                        //
                        value.data( data, size);


                        return CASUAL_FIELD_SUCCESS;
                     }

                  }

                  value = value.next();

               }

               return CASUAL_FIELD_NO_OCCURRENCE;

            }

            template<typename T>
            int update( const char* const handle, const long id, const long index, const int type, const T data)
            {
               const auto encoded = common::network::byteorder::encode( data);
               return update( handle, id, index, type, reinterpret_cast<const char*>( &encoded), sizeof (encoded));
            }

            int get( const char* const handle, const long id, long index, const int type, const char** data, long* size)
            {

               if( type != (id / CASUAL_FIELD_TYPE_BASE))
               {
                  return CASUAL_FIELD_INVALID_ID;
               }


               auto buffer = find_buffer( handle);

               if( ! buffer)
               {
                  return CASUAL_FIELD_INVALID_BUFFER;
               }

               //if( !data || !size)
               //{
               //   return CASUAL_FIELD_INVALID_ARGUMENT;
               //}


               const Value beyond( buffer->end());
               Value value( buffer->begin());

               while( value != beyond)
               {
                  if( value.id() == id)
                  {
                     if( ! index--)
                     {
                        if( data) *data = value.data();
                        if( size) *size = value.size();
                        return CASUAL_FIELD_SUCCESS;
                     }

                  }

                  value = value.next();

               }

               return CASUAL_FIELD_NO_OCCURRENCE;

            }

            template<typename T>
            int get( const char* const handle, const long id, const long index, const int type, T* const value)
            {
               const char* data = nullptr;

               if( const auto result = get( handle, id, index, type, &data, nullptr))
               {
                  return result;
               }

               if( value)
               {
                  const auto encoded = *reinterpret_cast< const common::network::byteorder::type<T>*>( data);
                  *value = common::network::byteorder::decode<T>( encoded);
               }

               return CASUAL_FIELD_SUCCESS;
            }

            int next( const char* const handle, long& id, long& index)
            {
               //
               // This may be inefficient for large buffers
               //
               // Perhaps we shall store the occurrence as well, but then
               // insertions and removals becomes a bit more cumbersome
               //
               // An other option is to make this and double-linked-list-like
               //

               const auto buffer = find_buffer( handle);

               if( ! buffer)
               {
                  return CASUAL_FIELD_INVALID_BUFFER;
               }

               const Value beyond( buffer->end());
               Value value( buffer->begin());

               //
               // Store all found id's to estimate index later
               //
               std::vector<decltype(value.id())> values;


               while( value != beyond)
               {
                  values.push_back( value.id());

                  value = value.next();

                  if( values.back() == id)
                  {
                     if( ! index--)
                     {
                        if( value != beyond)
                        {
                           id = value.id();
                           index = std::count( values.begin(), values.end(), id);

                           if( id == CASUAL_FIELD_NO_ID)
                           {
                              // We have encountered a removed occurrence
                              return next( handle, id, index);
                           }

                           return CASUAL_FIELD_SUCCESS;
                        }
                        else
                        {
                           return CASUAL_FIELD_NO_OCCURRENCE;
                        }
                     }

                  }

               }

               //
               // We couldn't even find the previous one
               //
               return CASUAL_FIELD_INVALID_ID;

            }

            int first( const char* const handle, long& id, long& index)
            {
               const auto buffer = find_buffer( handle);

               if( ! buffer)
               {
                  return CASUAL_FIELD_INVALID_BUFFER;
               }

               const Value value( buffer->begin());
               const Value beyond( buffer->end());

               if( value != beyond)
               {
                  id = value.id();
                  index = 0;

                  if( id == CASUAL_FIELD_NO_ID)
                  {
                     // The first field was removed
                     return next( handle, id, index);
                  }

               }
               else
               {
                  return CASUAL_FIELD_NO_OCCURRENCE;
               }

               return CASUAL_FIELD_SUCCESS;
            }


            int count( const char* const handle, const long id, long& occurrences)
            {
               const auto buffer = find_buffer( handle);

               if( ! buffer)
               {
                  return CASUAL_FIELD_INVALID_BUFFER;
               }

               const Value beyond( buffer->end());
               Value value( buffer->begin());

               occurrences = 0;

               while( value != beyond)
               {
                  if( value.id() == id)
                  {
                     ++occurrences;
                  }

                  value = value.next();
               }

               return CASUAL_FIELD_SUCCESS;

            }

            int count( const char* const handle, long& occurrences)
            {
               const auto buffer = find_buffer( handle);

               if( ! buffer)
               {
                  return CASUAL_FIELD_INVALID_BUFFER;
               }

               const Value beyond( buffer->end());
               Value value( buffer->begin());

               occurrences = 0;

               while( value != beyond)
               {
                  if( value.id() != CASUAL_FIELD_NO_ID)
                  {
                     ++occurrences;
                  }

                  value = value.next();
               }

               return CASUAL_FIELD_SUCCESS;

            }




            int reset( const char* const handle)
            {
               if( auto buffer = find_buffer( handle))
               {
                  buffer->utilized( 0);
               }
               else
               {
                  return CASUAL_FIELD_INVALID_BUFFER;
               }

               return CASUAL_FIELD_SUCCESS;

            }

            int explore( const char* const handle, long* const size, long* const used)
            {
               if( const auto buffer = find_buffer( handle))
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


            int copy( const char* target_handle, const char* source_handle)
            {
               const auto target = find_buffer( target_handle);

               const auto source = find_buffer( source_handle);

               if( target && source)
               {
                  if( target->reserved() < source->utilized())
                  {
                     return CASUAL_FIELD_NO_SPACE;
                  }

                  target->utilized( source->utilized());

                  //
                  // Copy the content
                  //

                  std::copy( source->begin(), source->end(), target->begin());

               }
               else
               {
                  return CASUAL_FIELD_INVALID_BUFFER;
               }

               return CASUAL_FIELD_SUCCESS;
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
   //
   // Perhaps we can use casual::buffer::field::next, but not yet
   //
   const int type = id / CASUAL_FIELD_TYPE_BASE;

   if( const auto result = casual::buffer::field::get( buffer, id, index, type, nullptr, count))
   {
      return result;
   }

   if( count)
   {
      switch( type)
      {
      case CASUAL_FIELD_STRING:
      case CASUAL_FIELD_BINARY:
         break;
      default:
         return CasualFieldPlainTypeHostSize( type, count);
      }
   }

   return CASUAL_FIELD_SUCCESS;

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
   const auto count = std::strlen( value) + 1;
   return casual::buffer::field::add( buffer, id, CASUAL_FIELD_STRING, value, count);
}

int CasualFieldAddBinary( char* const buffer, const long id, const char* const value, const long count)
{
   if( count < 0)
   {
      return CASUAL_FIELD_INVALID_ARGUMENT;
   }

   return casual::buffer::field::add( buffer, id, CASUAL_FIELD_BINARY, value, count);
}

int CasualFieldAddValue( char* const buffer, const long id, const void* const value, const long count)
{
   if( !value)
   {
      return CASUAL_FIELD_INVALID_ARGUMENT;
   }

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
   return casual::buffer::field::get( buffer, id, index, CASUAL_FIELD_CHAR, value);
}

int CasualFieldGetShort( const char* const buffer, const long id, const long index, short* const value)
{
   return casual::buffer::field::get( buffer, id, index, CASUAL_FIELD_SHORT, value);
}

int CasualFieldGetLong( const char* const buffer, const long id, const long index, long* const value)
{
   return casual::buffer::field::get( buffer, id, index, CASUAL_FIELD_LONG, value);
}

int CasualFieldGetFloat( const char* const buffer, const long id, const long index, float* const value)
{
   return casual::buffer::field::get( buffer, id, index, CASUAL_FIELD_FLOAT, value);
}

int CasualFieldGetDouble( const char* const buffer, const long id, const long index, double* const value)
{
   return casual::buffer::field::get( buffer, id, index, CASUAL_FIELD_DOUBLE, value);
}

int CasualFieldGetString( const char* const buffer, const long id, const long index, const char** value)
{
   return casual::buffer::field::get( buffer, id, index, CASUAL_FIELD_STRING, value, nullptr);
}

int CasualFieldGetBinary( const char* const buffer, const long id, const long index, const char** value, long* const count)
{
   return casual::buffer::field::get( buffer, id, index, CASUAL_FIELD_BINARY, value, count);
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
      if( const auto result = casual::buffer::field::get( buffer, id, index, type, &data, &size))
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
   const long count = std::strlen( value) + 1;
   return casual::buffer::field::update( buffer, id, index, CASUAL_FIELD_STRING, value, count);
}

int CasualFieldUpdateBinary( char* const buffer, const long id, const long index, const char* const value, const long count)
{
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
            std::map<std::string,int> name_to_type()
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

            std::map<int,std::string> type_to_name()
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

                  template< typename A>
                  void serialize( A& archive)
                  {
                     archive & CASUAL_MAKE_NVP( id);
                     archive & CASUAL_MAKE_NVP( name);
                     archive & CASUAL_MAKE_NVP( type);
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
                              throw common::exception::Base( "id for " + field.name + " is invalid");
                           }
                        }
                        catch( const std::out_of_range&)
                        {
                           // TODO: Much better
                           throw common::exception::Base( "type for " + field.name + " is invalid");
                        }
                     }
                  }

                  return fields;

               }

            } //

            std::map<std::string,long> name_to_id()
            {
               const auto fields = fetch_fields();

               decltype( name_to_id()) result;

               for( const auto& field : fields)
               {
                  if( ! result.emplace( field.name, field.id).second)
                  {
                     // TODO: Much better
                     throw common::exception::Base( "name for " + field.name + " is not unique");
                  }
               }

               return result;
            }

            std::map<long,std::string> id_to_name()
            {
               const auto fields = fetch_fields();

               decltype( id_to_name()) result;

               for( const auto& field : fields)
               {
                  if( ! result.emplace( field.id, field.name).second)
                  {
                     // TODO: Much better
                     throw common::exception::Base( "id for " + field.name + " is not unique");
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
   return casual::buffer::field::remove( buffer, id);
}

int CasualFieldRemoveOccurrence( char* const buffer, const long id, long index)
{
   return casual::buffer::field::remove( buffer, id, index);
}

int CasualFieldCopyBuffer( char* const target, const char* const source)
{
   return casual::buffer::field::copy( target, source);
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
                  const auto buffer = find_buffer( handle);

                  if( ! buffer)
                  {
                     return CASUAL_FIELD_INVALID_BUFFER;
                  }

                  const Value beyond( buffer->end());
                  Value value( buffer->begin());

                  stream << std::fixed;

                  std::map<long,long> occurrences;

                  while( value != beyond)
                  {
                     const auto id = value.id();

                     if( id != CASUAL_FIELD_NO_ID)
                     {
                        if( const auto name = repository::id_to_name( id))
                        {
                           stream << name;
                        }
                        else
                        {
                           stream << id;
                        }

                        stream << '[' << occurrences[id]++ << ']' << " = ";

                        switch( id / CASUAL_FIELD_TYPE_BASE)
                        {
                        case CASUAL_FIELD_SHORT:
                           stream << pod<short>( value.data());
                           break;
                        case CASUAL_FIELD_LONG:
                           stream << pod<long>( value.data());
                           break;
                        case CASUAL_FIELD_CHAR:
                           stream << *value.data();
                           break;
                        case CASUAL_FIELD_FLOAT:
                           stream << pod<float>( value.data());
                           break;
                        case CASUAL_FIELD_DOUBLE:
                           stream << pod<double>( value.data());
                           break;
                        // TODO: Handle string+binary in ... some way ... do we need escaping ?
                        case CASUAL_FIELD_STRING:
                        case CASUAL_FIELD_BINARY:
                        default:
                           stream << value.data();
                           break;
                        }

                        stream << '\n';

                     }

                     value = value.next();
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
      //
      // TODO: Log or remove this
      return CASUAL_FIELD_INTERNAL_FAILURE;
   }

   return CASUAL_FIELD_SUCCESS;
}
