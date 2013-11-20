//
// casual_field_buffer.cpp
//
//  Created on: 3 nov 2013
//      Author: Kristone
//

#include "common/field_buffer.h"

#include "common/network_byteorder.h"

#include <cstring>

#include <iostream>


namespace
{

   namespace local
   {
      using casual::common::network::byteorder;

      namespace explore
      {
         int type_from_id( const long id)
         {
            //
            // CASUAL_FIELD_SHORT is 0 'cause FLD_SHORT is 0 and thus we need
            // to represent invalid id with something else than 0
            //

            if( id > 0)
            {
               const int result = id / 0x200000;

               switch( result)
               {
               case CASUAL_FIELD_SHORT:
               case CASUAL_FIELD_LONG:
               case CASUAL_FIELD_CHAR:
               case CASUAL_FIELD_FLOAT:
               case CASUAL_FIELD_DOUBLE:
               case CASUAL_FIELD_STRING:
               case CASUAL_FIELD_BINARY:
                  return result;
               default:
                  break;
               }
            }

            return -1;

         }

         const char* name_from_type( const int type)
         {
            switch( type)
            {
            case CASUAL_FIELD_SHORT:
               return "FIELD_SHORT";
            case CASUAL_FIELD_LONG:
               return "FIELD_LONG";
            case CASUAL_FIELD_CHAR:
               return "FIELD_CHAR";
            case CASUAL_FIELD_FLOAT:
               return "FIELD_FLOAT";
            case CASUAL_FIELD_DOUBLE:
               return "FIELD_DOUBLE";
            case CASUAL_FIELD_STRING:
               return "FIELD_STRING";
            case CASUAL_FIELD_BINARY:
               return "FIELD_BINARY";
            default:
               return nullptr;
            }
         }

         const char* name_from_id( const long id)
         {
            // TODO
            return nullptr;
         }

         long id_from_name( const char* const name)
         {
            // TODO
            return 0;
         }

      }


      namespace arithmetic
      {
         template<typename T>
         inline void write( char* const where, const T& value)
         {
            const auto encoded = byteorder<T>::encode( value);
            std::memcpy( where, &encoded, sizeof( encoded));
         }

         template<typename T>
         inline T parse( const char* const where)
         {
            return byteorder<T>::decode( *reinterpret_cast< const decltype(byteorder<T>::encode(0))*>( where));
         }

         template< typename T>
         constexpr long bytes()
         {
            return sizeof(decltype(byteorder<T>::encode(0)));
         }

      }

      namespace field
      {
         enum position : long
         {
            id, count, value
         };

         // minimal size
         constexpr auto size() -> decltype(arithmetic::bytes<long>())
         {
            return arithmetic::bytes< long>() * position::value;
         }

         namespace insert
         {
            void id( char* const selector, const long id)
            {
               arithmetic::write( selector + arithmetic::bytes<long>() * position::id, id);
            }

            void count( char* const selector, const long count)
            {
               arithmetic::write( selector + arithmetic::bytes<long>() * position::count, count);
            }

            // @return the offset
            char* value( char* const selector)
            {
               return selector + arithmetic::bytes<long>() * position::value;
            }

         }

         namespace select
         {
            long id( const char* const selector)
            {
               return arithmetic::parse<long>( selector + arithmetic::bytes<long>() * position::id);
            }

            long count( const char* const selector)
            {
               return arithmetic::parse<long>( selector + arithmetic::bytes<long>() * position::count);
            }

            // @return the offset
            const char* value( const char* const selector)
            {
               return selector + arithmetic::bytes<long>() * position::value;
            }
         }

      }


      namespace header
      {
         enum position : long
         {
            reserved, inserter, beoyond
         };

         constexpr auto size() -> decltype(arithmetic::bytes<long>())
         {
            return arithmetic::bytes< long>() * position::beoyond;
         }

         namespace select
         {
            long reserved( const char* const buffer)
            {
               return arithmetic::parse<long>( buffer + arithmetic::bytes<long>() * position::reserved);
            }

            long inserter( const char* const buffer)
            {
               return arithmetic::parse<long>( buffer + arithmetic::bytes<long>() * position::inserter);
            }
         }

         namespace update
         {
            void reserved( char* const buffer, const long value)
            {
               arithmetic::write<long>( buffer + arithmetic::bytes< long>() * position::reserved, value);
            }

            void inserter( char* const buffer, const long value)
            {
               arithmetic::write<long>( buffer + arithmetic::bytes< long>() * position::inserter, value);
            }

         }
      }

      template<typename T>
      struct pod_traits{};

      template< > struct pod_traits< char>
      {
         static const auto type = CASUAL_FIELD_CHAR;
      };
      template< > struct pod_traits< short>
      {
         static const auto type = CASUAL_FIELD_SHORT;
      };
      template< > struct pod_traits< long>
      {
         static const auto type = CASUAL_FIELD_LONG;
      };
      template< > struct pod_traits< float>
      {
         static const auto type = CASUAL_FIELD_FLOAT;
      };
      template< > struct pod_traits< double>
      {
         static const auto type = CASUAL_FIELD_DOUBLE;
      };

      namespace add
      {

         //
         // TODO: make pod+string+binary a bit more generic and remove some ugliness
         //

         char* prepare( char* const buffer, const long id, const long count)
         {

            const auto reserved = header::select::reserved( buffer);
            const auto inserter = header::select::inserter( buffer);

            // we need room for id, count and the value it self
            const auto total = field::size() + count;

            if( (reserved - inserter) < total)
            {
               return nullptr;
            }

            field::insert::id( buffer + inserter, id);
            field::insert::count( buffer + inserter, count);

            header::update::inserter( buffer, inserter + total);

            return buffer + inserter;

         }

         template<typename T>
         int pod( char* const buffer, const long id, const T& value)
         {
            if( explore::type_from_id( id) != pod_traits<T>::type)
            {
               return CASUAL_FIELD_INVALID_ID;
            }

            if( char* const inserter = prepare( buffer, id, arithmetic::bytes<T>()))
            {
               arithmetic::write( field::insert::value( inserter), value);
            }
            else
            {
               return CASUAL_FIELD_NO_SPACE;
            }

            return CASUAL_FIELD_SUCCESS;

         }

         int string( char* buffer, const long id, const char* const value)
         {
            if( explore::type_from_id( id) != CASUAL_FIELD_STRING)
            {
               return CASUAL_FIELD_INVALID_ID;
            }

            const auto count = std::strlen( value) + 1;

            if( char* const inserter = prepare( buffer, id, count))
            {
               std::memcpy( field::insert::value( inserter), value, count);
            }
            else
            {
               return CASUAL_FIELD_NO_SPACE;
            }

            return CASUAL_FIELD_SUCCESS;

         }

         int binary( char* buffer, const long id, const char* const value, const long count)
         {
            if( explore::type_from_id( id) != CASUAL_FIELD_BINARY)
            {
               return CASUAL_FIELD_INVALID_ID;
            }

            if( char* const inserter = prepare( buffer, id, count))
            {
               std::memcpy( field::insert::value( inserter), value, count);
            }
            else
            {
               return CASUAL_FIELD_NO_SPACE;
            }

            return CASUAL_FIELD_SUCCESS;
         }


      }

      namespace get
      {

         //
         // TODO: make pod+string+binary a bit more generic and remove some ugliness
         //

         const char* prepare( const char* const buffer, const long id, long index)
         {

            const char* const inserter = buffer + header::select::inserter( buffer);
            const char* selector = buffer + header::size();

            while( selector < inserter)
            {

               if( field::select::id( selector) == id)
               {
                  if( !index--)
                  {
                     return selector;
                  }
               }

               selector += field::size() + field::select::count( selector);

            }

            return nullptr;

         }

         template<typename T>
         int pod( const char* const buffer, const long id, const long index, T& value)
         {
            if( explore::type_from_id( id) != pod_traits<T>::type)
            {
               return CASUAL_FIELD_INVALID_ID;
            }

            if( const char* const selector = prepare( buffer, id, index))
            {
               value = arithmetic::parse<T>( field::select::value( selector));
            }
            else
            {
               return CASUAL_FIELD_NO_OCCURRENCE;
            }

            return CASUAL_FIELD_SUCCESS;
         }

         int string( const char* const buffer, const long id, const long index, const char*& value)
         {
            if( explore::type_from_id( id) != CASUAL_FIELD_STRING)
            {
               return CASUAL_FIELD_INVALID_ID;
            }

            if( const char* const selector = prepare( buffer, id, index))
            {
               value = field::select::value( selector);
            }
            else
            {
               return CASUAL_FIELD_NO_OCCURRENCE;
            }

            return CASUAL_FIELD_SUCCESS;
         }

         int binary( const char* const buffer, const long id, const long index, const char*& value, long& count)
         {
            if( explore::type_from_id( id) != CASUAL_FIELD_BINARY)
            {
               return CASUAL_FIELD_INVALID_ID;
            }

            if( const char* const selector = prepare( buffer, id, index))
            {
               value = field::select::value( selector);
               count = field::select::count( selector);
            }
            else
            {
               return CASUAL_FIELD_NO_OCCURRENCE;
            }

            return CASUAL_FIELD_SUCCESS;
         }

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
      case CASUAL_FIELD_INVALID_TYPE:
         return "Invalid type";
      case CASUAL_FIELD_INTERNAL_FAILURE:
         return "Internal failure";
      default:
         return "Unknown code";
   }
}

int CasualFieldAddChar( char* const buffer, const long id, const char value)
{
   return local::add::pod( buffer, id, value);
}

int CasualFieldAddShort( char* const buffer, const long id, const short value)
{
   return local::add::pod( buffer, id, value);
}

int CasualFieldAddLong( char* const buffer, const long id, const long value)
{
   return local::add::pod( buffer, id, value);
}
int CasualFieldAddFloat( char* const buffer, const long id, const float value)
{
   return local::add::pod( buffer, id, value);
}

int CasualFieldAddDouble( char* const buffer, const long id, const double value)
{
   return local::add::pod( buffer, id, value);
}

int CasualFieldAddString( char* const buffer, const long id, const char* const value)
{
   return local::add::string( buffer, id, value);
}

int CasualFieldAddBinary( char* const buffer, const long id, const char* const value, const long count)
{
   return local::add::binary( buffer, id, value, count);
}

int CasualFieldGetChar( const char* const buffer, const long id, const long index, char* const value)
{
   return local::get::pod( buffer, id, index, *value);
}

int CasualFieldGetShort( const char* const buffer, const long id, const long index, short* const value)
{
   return local::get::pod( buffer, id, index, *value);
}

int CasualFieldGetLong( const char* const buffer, const long id, const long index, long* const value)
{
   return local::get::pod( buffer, id, index, *value);
}

int CasualFieldGetFloat( const char* const buffer, const long id, const long index, float* const value)
{
   return local::get::pod( buffer, id, index, *value);
}

int CasualFieldGetDouble( const char* const buffer, const long id, const long index, double* const value)
{
   return local::get::pod( buffer, id, index, *value);
}

int CasualFieldGetString( const char* const buffer, const long id, const long index, const char** value)
{
   return local::get::string( buffer, id, index, *value);
}

int CasualFieldGetBinary( const char* const buffer, const long id, const long index, const char** value, long* const count)
{
   return local::get::binary( buffer, id, index, *value, *count);
}


int CasualFieldExploreBuffer( const char* buffer, long* const size, long* const used)
{
   if( size != nullptr)
   {
      *size = local::header::select::reserved( buffer);
   }

   if( used != nullptr)
   {
      *used = local::header::select::inserter( buffer);
   }

   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldNameOfId( long id, const char** name)
{
   // TODO

   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldIdOfName( const char* name, long* const id)
{
   // TODO

   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldTypeOfId( const long id, int* const type)
{
   const int result = local::explore::type_from_id( id);

   if( result < 0)
   {
      return CASUAL_FIELD_INVALID_ID;
   }

   *type = result;

   return CASUAL_FIELD_SUCCESS;

}

int CasualFieldNameOfType( const int type, const char** name)
{
   if( const char* const result = local::explore::name_from_type( type))
   {
      *name = result;
   }
   else
   {
      return CASUAL_FIELD_INVALID_TYPE;
   }

   return CASUAL_FIELD_SUCCESS;
}


int CasualFieldExist( const char* const buffer, const long id, const long occurrence)
{
   if( local::explore::type_from_id( id) < 0)
   {
      return CASUAL_FIELD_INVALID_ID;
   }

   const char* const selector = local::get::prepare( buffer, id, occurrence);

   return selector != nullptr ? CASUAL_FIELD_SUCCESS : CASUAL_FIELD_NO_OCCURRENCE;
}

int CasualFieldRemoveAll( char* const buffer)
{
   // just move the inserter to the beginning
   local::header::update::inserter( buffer, local::header::size());

   return CASUAL_FIELD_SUCCESS;
}

int CasualFieldRemoveId( char* buffer, const long id)
{

   if( const int result = CasualFieldExist( buffer, id, 0))
   {
      return result;
   }

   while( CasualFieldRemoveOccurrence( buffer, id, 0) == CASUAL_FIELD_SUCCESS)
   {}

   return CASUAL_FIELD_SUCCESS;

}


int CasualFieldRemoveOccurrence( char* const buffer, const long id, long occurrence)
{
   if( const int result = CasualFieldExist( buffer, id, 0))
   {
      return result;
   }

   if( const char* const selector = local::get::prepare( buffer, id, occurrence))
   {
      // const_cast by fooling the compiler (but that is what we want)
      char* const inserter = buffer + (selector - buffer);

      local::field::insert::id( inserter, 0);
   }
   else
   {
      return CASUAL_FIELD_NO_OCCURRENCE;
   }

   return CASUAL_FIELD_SUCCESS;

}


int CasualFieldCopyBuffer( char* const target, const char* const source)
{

   const auto reserved = local::header::select::reserved( target);
   const auto inserter = local::header::select::inserter( source);

   if( reserved < inserter)
   {
      return CASUAL_FIELD_NO_SPACE;
   }

   constexpr auto bytes = local::arithmetic::bytes< long>();

   // copy inserter and selector (and data) but leave reserved
   std::memcpy( target + bytes, source + bytes, inserter - bytes);

   return CASUAL_FIELD_SUCCESS;
}


long CasualFieldCreate( char* const buffer, const long size)
{
   constexpr auto bytes = local::header::size();

   if( size < bytes)
   {
      return CASUAL_FIELD_NO_SPACE;
   }

   local::header::update::reserved( buffer, size);
   local::header::update::inserter( buffer, bytes);

   return CASUAL_FIELD_SUCCESS;
}

long CasualFieldExpand( char* const buffer, const long size)
{
   local::header::update::reserved( buffer, size);

   return CASUAL_FIELD_SUCCESS;
}

long CasualFieldReduce( char* const buffer, const long size)
{
   const auto inserter = local::header::select::inserter( buffer);

   if( size < inserter)
   {
      return CASUAL_FIELD_NO_SPACE;
   }

   local::header::update::reserved( buffer, size);

   return CASUAL_FIELD_SUCCESS;
}

long CasualFieldNeeded( char* const buffer, const long size)
{
   return local::header::select::inserter( buffer);
}

