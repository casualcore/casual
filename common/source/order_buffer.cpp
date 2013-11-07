//
// casual_order_buffer.cpp
//
//  Created on: 20 okt 2013
//      Author: Kristone
//

#include "common/order_buffer.h"

#include "common/network_byteorder.h"

#include <cstring>

namespace casual
{
   namespace common
   {
      namespace buffer
      {
         namespace
         {
            namespace local
            {

               namespace data
               {

                  template< typename T>
                  constexpr long bytes()
                  {
                     return sizeof(decltype(network::byteorder<T>::encode(0)));
                  }

                  template< typename T>
                  T select( const char* const buffer, const long offset)
                  {
                     return network::byteorder<T>::decode( *reinterpret_cast< const decltype(network::byteorder<T>::encode(0))*>( buffer + offset));
                  }

                  template< typename T>
                  void insert( char* const buffer, const long offset, const T value)
                  {
                     const auto encoded = network::byteorder<T>::encode( value);
                     std::memcpy( buffer + offset, &encoded, sizeof( encoded));
                  }

               }

               namespace header
               {
                  enum position : long
                  {
                     reserved, inserter, selector, beoyond
                  };

                  constexpr auto size() -> decltype(data::bytes<long>())
                  {
                     return data::bytes< long>() * position::beoyond;
                  }

                  namespace select
                  {
                     template< position offset>
                     long parse( const char* const buffer)
                     {
                        return data::select< long>( buffer, data::bytes< long>() * offset);
                     }

                     long reserved( const char* const buffer)
                     {
                        return parse< position::reserved>( buffer);
                     }

                     long inserter( const char* const buffer)
                     {
                        return parse< position::inserter>( buffer);
                     }

                     long selector( const char* const buffer)
                     {
                        return parse< position::selector>( buffer);
                     }
                  }

                  namespace update
                  {
                     template< position offset>
                     void write( char* const buffer, const long value)
                     {
                        data::insert< long>( buffer, data::bytes< long>() * offset, value);
                     }

                     void reserved( char* const buffer, const long value)
                     {
                        write< position::reserved>( buffer, value);
                     }

                     void inserter( char* const buffer, const long value)
                     {
                        write< position::inserter>( buffer, value);
                     }

                     void selector( char* const buffer, const long value)
                     {
                        write< position::selector>( buffer, value);
                     }

                  }
               }

               int add( char* const buffer, const char* const value, const long length)
               {
                  const auto reserved = header::select::reserved( buffer);
                  const auto inserter = header::select::inserter( buffer);

                  constexpr auto length_size = data::bytes< long>();

                  if( ( reserved - inserter) < ( length_size + length))
                  {
                     return CASUAL_ORDER_NO_SPACE;
                  }

                  data::insert< long>( buffer, inserter, length);

                  std::memcpy( buffer + inserter + length_size, value, length);

                  header::update::inserter( buffer, inserter + length_size + length);

                  return CASUAL_ORDER_SUCCESS;
               }

               template< typename T>
               int add( char* const buffer, const T& value)
               {
                  const auto reserved = header::select::reserved( buffer);
                  const auto inserter = header::select::inserter( buffer);

                  constexpr auto value_size = data::bytes< T>();

                  if( ( reserved - inserter) < ( value_size))
                  {
                     return CASUAL_ORDER_NO_SPACE;
                  }

                  data::insert< T>( buffer, inserter, value);

                  header::update::inserter( buffer, inserter + value_size);

                  return CASUAL_ORDER_SUCCESS;
               }

               template< typename T>
               int get( char* const buffer, T& value)
               {
                  const auto inserter = header::select::inserter( buffer);
                  const auto selector = header::select::selector( buffer);

                  constexpr auto value_size = data::bytes< T>();

                  if( ( inserter - selector) < ( value_size))
                  {
                     return CASUAL_ORDER_NO_PLACE;
                  }

                  value = data::select< T>( buffer, selector);

                  header::update::selector( buffer, selector + value_size);

                  return CASUAL_ORDER_SUCCESS;
               }

               int get( char* const buffer, const char*& value, long& length)
               {
                  const auto inserter = header::select::inserter( buffer);
                  const auto selector = header::select::selector( buffer);

                  constexpr auto length_size = data::bytes< long>();

                  if( ( inserter - selector) < ( length_size))
                  {
                     // not even the size can be read so we need to report that
                     return CASUAL_ORDER_NO_PLACE;
                  }

                  const auto value_size = data::select< long>( buffer, selector);

                  if( ( inserter - selector) < ( length_size + value_size))
                  {
                     // the size is impossible so we need to report that
                     return CASUAL_ORDER_NO_PLACE;
                  }

                  value = buffer + selector + length_size;
                  length = value_size;

                  header::update::selector( buffer, selector + length_size + value_size);

                  return CASUAL_ORDER_SUCCESS;
               }

            }
         }
      }

   }

}

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
      case CASUAL_ORDER_INTERNAL_FAILURE:
         return "Internal failure";
      default:
         return "Uknown code";
   }
}

int CasualOrderAddPrepare( char* const buffer)
{
   casual::common::buffer::local::header::update::inserter( buffer, casual::common::buffer::local::header::size());

   return CASUAL_ORDER_SUCCESS;
}

int CasualOrderAddBool( char* const buffer, const bool value)
{
   return casual::common::buffer::local::add( buffer, value);
}

int CasualOrderAddChar( char* const buffer, const char value)
{
   return casual::common::buffer::local::add( buffer, value);
}

int CasualOrderAddShort( char* const buffer, const short value)
{
   return casual::common::buffer::local::add( buffer, value);
}

int CasualOrderAddLong( char* const buffer, const long value)
{
   return casual::common::buffer::local::add( buffer, value);
}

int CasualOrderAddFloat( char* const buffer, const float value)
{
   return casual::common::buffer::local::add( buffer, value);
}

int CasualOrderAddDouble( char* const buffer, const double value)
{
   return casual::common::buffer::local::add( buffer, value);
}

int CasualOrderAddString( char* const buffer, const char* const value)
{
   return casual::common::buffer::local::add( buffer, value, std::strlen( value) + 1);
}

int CasualOrderAddBinary( char* const buffer, const char* const value, const long size)
{
   return casual::common::buffer::local::add( buffer, value, size);
}

int CasualOrderGetPrepare( char* const buffer)
{
   casual::common::buffer::local::header::update::selector( buffer, casual::common::buffer::local::header::size());

   return CASUAL_ORDER_SUCCESS;
}

int CasualOrderGetBool( char* const buffer, bool* const value)
{
   return casual::common::buffer::local::get( buffer, *value);
}

int CasualOrderGetChar( char* const buffer, char* const value)
{
   return casual::common::buffer::local::get( buffer, *value);
}

int CasualOrderGetShort( char* const buffer, short* const value)
{
   return casual::common::buffer::local::get( buffer, *value);
}

int CasualOrderGetLong( char* const buffer, long* const value)
{
   return casual::common::buffer::local::get( buffer, *value);
}

int CasualOrderGetFloat( char* const buffer, float* const value)
{
   return casual::common::buffer::local::get( buffer, *value);
}

int CasualOrderGetDouble( char* const buffer, double* const value)
{
   return casual::common::buffer::local::get( buffer, *value);
}

int CasualOrderGetString( char* const buffer, const char** value)
{
   long size;
   return casual::common::buffer::local::get( buffer, *value, size);
}

int CasualOrderGetBinary( char* const buffer, const char** value, long* const size)
{
   return casual::common::buffer::local::get( buffer, *value, *size);
}

int CasualOrderCopyBuffer( char* const target, const char* const source)
{
   const auto target_reserved = casual::common::buffer::local::header::select::reserved( target);
   const auto source_inserter = casual::common::buffer::local::header::select::inserter( source);

   if( target_reserved < source_inserter)
   {
      return CASUAL_ORDER_NO_SPACE;
   }

   constexpr auto bytes = casual::common::buffer::local::data::bytes< long>();

   // copy inserter and selector (and data) but leave reserved
   std::memcpy( target + bytes, source + bytes, source_inserter - bytes);

   return CASUAL_ORDER_SUCCESS;
}

int CasualOrderUsed( const char* const buffer, long* const size)
{
   *size = casual::common::buffer::local::header::select::inserter( buffer);
   return CASUAL_ORDER_SUCCESS;
}

long CasualOrderCreate( char* const buffer, const long size)
{
   constexpr auto bytes = casual::common::buffer::local::header::size();

   if( size < bytes)
   {
      return CASUAL_ORDER_NO_SPACE;
   }

   casual::common::buffer::local::header::update::reserved( buffer, size);
   casual::common::buffer::local::header::update::inserter( buffer, bytes);
   casual::common::buffer::local::header::update::selector( buffer, bytes);

   return CASUAL_ORDER_SUCCESS;
}

long CasualOrderExpand( char* const buffer, const long size)
{
   casual::common::buffer::local::header::update::reserved( buffer, size);

   return CASUAL_ORDER_SUCCESS;
}

long CasualOrderReduce( char* const buffer, const long size)
{
   const auto inserter = casual::common::buffer::local::header::select::inserter( buffer);

   if( size < inserter)
   {
      return CASUAL_ORDER_NO_SPACE;
   }

   casual::common::buffer::local::header::update::reserved( buffer, size);

   return CASUAL_ORDER_SUCCESS;
}

long CasualOrderNeeded( char* const buffer, const long size)
{
   return casual::common::buffer::local::header::select::inserter( buffer);
}

