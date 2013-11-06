//
// casual_field_buffer.cpp
//
//  Created on: 3 nov 2013
//      Author: Kristone
//

#include "common/field_buffer.h"

#include "common/network.h"

#include <cstring>

namespace
{
   namespace internal
   {
      namespace explore
      {
         int type( const long id)
         {
            const int result = id / 100000;

            if( result < CASUAL_FIELD_BOOL || result > CASUAL_FIELD_BINARY)
            {
               return 0;
            }

            return result;
         }

         const char* name( const long id)
         {
            // TODO
            return nullptr;
         }

         long id( const char* const name)
         {
            // TODO
            return 0;
         }

      }
      namespace data
      {

         template< typename T>
         auto encode( const T value) -> decltype(casual::common::network::transcoder<T>::encode(T()))
         {
            return casual::common::network::transcoder< T>::encode( value);
         }

         template< typename T>
         T decode( const decltype(casual::common::network::transcoder<T>::encode(T())) value)
         {
            return casual::common::network::transcoder< T>::decode( value);
         }

         template<typename T>
         constexpr long bytes()
         {
            return sizeof(decltype(encode<T>(0)));
         }

         template< typename T>
         T select( const char* const buffer, const long offset)
         {
            return decode< T>( *reinterpret_cast< const decltype(encode<T>(0))*>( buffer + offset));
         }

         template< typename T>
         void insert( char* const buffer, const long offset, const T value)
         {
            const auto encoded = encode< T>( value);
            std::memcpy( buffer + offset, &encoded, sizeof( encoded));
         }

      }

      namespace header
      {
         enum position : long
         {
            reserved,
            inserter,
            beoyond
         };

         constexpr long size()
         {
            return data::bytes<long>() * position::beoyond;
         }

         namespace select
         {
            template< position offset>
            long parse( const char* const buffer)
            {
               return data::select<long>( buffer, data::bytes<long>() * offset);
            }

            long reserved( const char* const buffer)
            {
               return parse< position::reserved>( buffer);
            }

            long inserter( const char* const buffer)
            {
               return parse< position::inserter>( buffer);
            }

         }

         namespace update
         {
            template< position offset>
            void write( char* const buffer, const long value)
            {
               data::insert< long>( buffer, data::bytes<long>() * offset, value);
            }

            void reserved( char* const buffer, const long value)
            {
               write< position::reserved>( buffer, value);
            }

            void inserter( char* const buffer, const long value)
            {
               write< position::inserter>( buffer, value);
            }

         }
      }

      int add( char* const buffer, const long id, const int type, const void* const value, const long length)
      {
         if( explore::type( id) != type)
         {
            return CASUAL_FIELD_INVALID_ID;
         }

         if( explore::name( id) == nullptr)
         {
            return CASUAL_FIELD_UNKNOWN_ID;
         }

         const auto reserved = header::select::reserved( buffer);
         const auto inserter = header::select::inserter( buffer);

         constexpr auto id_size = data::bytes<decltype(id)>();
         constexpr auto length_size = data::bytes<decltype(length)>();

         if( ( reserved - inserter) < ( id_size + length_size + length))
         {
            return CASUAL_FIELD_NO_SPACE;
         }

         auto offset = inserter;

         data::insert( buffer, offset, id);
         offset += id_size;
         data::insert( buffer, offset, length);
         offset += length_size;
         std::memcpy( buffer + offset, value, length);
         offset += length;

         header::update::inserter( buffer, offset);

         return CASUAL_FIELD_SUCCESS;
      }

      template< typename T>
      int add( char* const buffer, const long id, const int type, const T& value)
      {
         const auto encoded_value = data::encode< T>( value);
         return add( buffer, id, type, &encoded_value, sizeof( encoded_value));
      }

   }
}

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
      case CASUAL_FIELD_INVALID_ID:
         return "Invalid id";
      case CASUAL_FIELD_INTERNAL_FAILURE:
         return "Internal failure";
      default:
         return "Unknown code";
   }
}

int CasualFieldAddBool( char* const buffer, const long id, const bool value)
{
   return internal::add( buffer, id, CASUAL_FIELD_BOOL, value);
}

int CasualFieldAddChar( char* const buffer, const long id, const char value)
{
   return internal::add( buffer, id, CASUAL_FIELD_CHAR, value);
}

int CasualFieldAddShort( char* const buffer, const long id, const short value)
{
   return internal::add( buffer, id, CASUAL_FIELD_SHORT, value);
}

int CasualFieldAddLong( char* const buffer, const long id, const long value)
{
   return internal::add( buffer, id, CASUAL_FIELD_LONG, value);
}
int CasualFieldAddFloat( char* const buffer, const long id, const float value)
{
   return internal::add( buffer, id, CASUAL_FIELD_FLOAT, value);
}

int CasualFieldAddDouble( char* const buffer, const long id, const double value)
{
   return internal::add( buffer, id, CASUAL_FIELD_DOUBLE, value);
}

int CasualFieldAddString( char* const buffer, const long id, const char* const value)
{
   return internal::add( buffer, id, CASUAL_FIELD_STRING, value, std::strlen( value) + 1);
}

int CasualFieldAddBinary( char* const buffer, const long id, const char* const value, const long size)
{
   return internal::add( buffer, id, CASUAL_FIELD_BINARY, value, size);
}

int CasualFieldAddData( char* const buffer, const long id, const void* const value, const long size)
{
   switch( internal::explore::type( id))
   {
      case CASUAL_FIELD_BOOL:
         return CasualFieldAddBool( buffer, id, *reinterpret_cast< const bool*>( value));
      case CASUAL_FIELD_CHAR:
         return CasualFieldAddChar( buffer, id, *reinterpret_cast< const char*>( value));
      case CASUAL_FIELD_SHORT:
         return CasualFieldAddShort( buffer, id, *reinterpret_cast< const short*>( value));
      case CASUAL_FIELD_LONG:
         return CasualFieldAddLong( buffer, id, *reinterpret_cast< const long*>( value));
      case CASUAL_FIELD_FLOAT:
         return CasualFieldAddFloat( buffer, id, *reinterpret_cast< const float*>( value));
      case CASUAL_FIELD_DOUBLE:
         return CasualFieldAddDouble( buffer, id, *reinterpret_cast< const double*>( value));
      case CASUAL_FIELD_STRING:
         return CasualFieldAddString( buffer, id, reinterpret_cast< const char*>( value));
      case CASUAL_FIELD_BINARY:
         return CasualFieldAddBinary( buffer, id, reinterpret_cast< const char*>( value), size);
      default:
         return CASUAL_FIELD_INVALID_ID;
   }

   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldGetBool( const char* const buffer, const long id, const long index, bool* const value)
{
   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldGetChar( const char* const buffer, const long id, const long index, char* const value)
{
   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldGetShort( const char* const buffer, const long id, const long index, short* const value)
{
   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldGetLong( const char* const buffer, const long id, const long index, long* const value)
{
   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldGetFloat( const char* const buffer, const long id, const long index, float* const value)
{
   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldGetDouble( const char* const buffer, const long id, const long index, double* const value)
{
   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldGetString( const char* const buffer, const long id, const long index, char** value)
{
   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldGetBinary( const char* const buffer, const long id, const long index, char** value, long* const size)
{
   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldGetData( const char* const buffer, const long id, const long index, void** value, long* const size)
{
   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldExploreBuffer( const char* buffer, long* const size, long* const used)
{
   if( size != nullptr)
   {
      *size = internal::header::select::reserved( buffer);
   }

   if( used != nullptr)
   {
      *used = internal::header::select::inserter( buffer);
   }

   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldExploreId( const char* const name, long* const id)
{
   if( const int result = internal::explore::id( name))
   {
      *id = result;
   }
   else
   {
      return CASUAL_FIELD_UNKNOWN_ID;
   }

   return CASUAL_FIELD_SUCCESS;

}

int CasualFieldExploreName( const long id, const char** name)
{
   if( const char* const result = internal::explore::name( id))
   {
      *name = result;
   }
   else
   {
      return CASUAL_FIELD_UNKNOWN_ID;
   }

   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldExploreType( const long id, int* const type)
{
   if( const int result = internal::explore::type( id))
   {
      if( internal::explore::name( id) != nullptr)
      {
         *type = result;
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

int CasualFieldRemoveAll( char* const buffer)
{
   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldRemoveId( char* const buffer, const long id)
{
   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldRemoveOccurrence( char* const buffer, const long id, const long occurrence)
{
   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldCopyBuffer( char* const target, const char* const source)
{
   return CASUAL_FIELD_SUCCESS;
}


long CasualFieldCreate( char* const buffer, const long size)
{
   constexpr auto bytes = internal::header::size();

   if( size < bytes)
   {
      return CASUAL_FIELD_NO_SPACE;
   }

   internal::header::update::reserved( buffer, size);
   internal::header::update::inserter( buffer, bytes);

   return CASUAL_FIELD_SUCCESS;
}

long CasualFieldExpand( char* const buffer, const long size)
{
   internal::header::update::reserved( buffer, size);

   return CASUAL_FIELD_SUCCESS;
}

long CasualFieldReduce( char* const buffer, const long size)
{
   const auto inserter = internal::header::select::inserter( buffer);

   if( size < inserter)
   {
      return CASUAL_FIELD_NO_SPACE;
   }

   internal::header::update::reserved( buffer, size);

   return CASUAL_FIELD_SUCCESS;
}

long CasualFieldNeeded( char* const buffer, const long size)
{
   return internal::header::select::inserter( buffer);
}

