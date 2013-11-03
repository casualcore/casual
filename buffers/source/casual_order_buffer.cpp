//
// casual_order_buffer.cpp
//
//  Created on: 20 okt 2013
//      Author: Kristone
//




#include <cstring>

#include "casual_order_buffer.h"

//#include <arpa/inet.h>
//#include <netinet/in.h>


namespace
{
   namespace internal
   {
      const char* description( const int code)
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

      template<typename T>
      struct transcoder
      {
         static T encode( const T value) {return value;}
         static T decode( const T value) {return value;}
      };

/*
      template<>
      struct transcoder<short>
      {
         // TODO: somehow use decltype
         static uint16_t encode( const short value){return htons(value);}
         static short decode( const uint16_t value){return ntohs(value);}
      };

      template<>
      struct transcoder<long>
      {
         // TODO: somehow use decltype
         static uint32_t encode( const long value){return htonl(value);}
         static long decode( const uint32_t value){return ntohl(value);}
      };
*/


      template<typename T>
      auto encode( const T value) -> decltype(transcoder<T>::encode(T()))
      {
         return transcoder<T>::encode( value);
      }


      template<typename T>
      T decode( const decltype(transcoder<T>::encode(T())) value)
      {
         return transcoder<T>::decode( value);
      }

      template<typename T>
      constexpr auto bytes() -> decltype(sizeof(decltype(encode<T>(T()))))
      {
         return sizeof(decltype(encode<T>(T())));
      }

      template<typename T>
      auto validate( const long offset, const long beyond) -> decltype(bytes<T>())
      {
         return (beyond - offset) < bytes<T>() ? 0 : bytes<T>();
      }

      namespace data
      {
         template<typename T>
         T select( const char* const buffer, const long offset)
         {
            return decode<T>( *reinterpret_cast<const decltype(encode<T>(T()))*>(buffer + offset));
         }


         template<typename T>
         void insert( char* const buffer, const long offset, const T& value)
         {
            const auto encoded = encode<T>( value);
            std::memcpy( buffer + offset, &encoded, sizeof(encoded));
         }

      }

      namespace header
      {
         namespace select
         {
            long reserved( const char* const buffer)
            {
               return data::select<long>( buffer, bytes<long>() * 0);
            }

            long inserter( const char* const buffer)
            {
               return data::select<long>( buffer, bytes<long>() * 1);
            }

            long selector( const char* const buffer)
            {
               return data::select<long>( buffer, bytes<long>() * 2);
            }
         }

         namespace update
         {
            void reserved( char* const buffer, const long value)
            {
               data::insert<long>( buffer, bytes<long>() * 0, value);
            }

            void inserter( char* const buffer, const long value)
            {
               data::insert<long>( buffer, bytes<long>() * 1, value);
            }

            void selector( char* const buffer, const long value)
            {
               data::insert<long>( buffer, bytes<long>() * 2, value);
            }

         }
      }


      template<typename T>
      int add( char* const buffer, const T& value)
      {
         const auto reserved = header::select::reserved( buffer);
         const auto inserter = header::select::inserter( buffer);

         if( const auto size = validate<T>( inserter, reserved))
         {
            data::insert<T>( buffer, inserter, value);
            header::update::inserter( buffer, inserter + size);
         }
         else
         {
            return CASUAL_ORDER_NO_SPACE;
         }

         return CASUAL_ORDER_SUCCESS;
      }


      int add( char* const buffer, const char* const value, const long length)
      {
         constexpr auto size_size = bytes<long>();

         const auto total = size_size + length;

         const auto reserved = header::select::reserved( buffer);
         const auto inserter = header::select::inserter( buffer);

         if( (reserved - inserter) < (size_size + length))
         {
            return CASUAL_ORDER_NO_SPACE;
         }

         data::insert<long>( buffer, inserter, length);

         std::memcpy( buffer + inserter + size_size, value, length);

         header::update::inserter( buffer, inserter + size_size + length);

         return CASUAL_ORDER_SUCCESS;
      }


      template<typename T>
      int get( char* const buffer, T& value)
      {
         const auto inserter = header::select::inserter( buffer);
         const auto selector = header::select::selector( buffer);


         if( const auto size = validate<T>( selector, inserter))
         {
            value = data::select<T>( buffer, selector);
            header::update::selector( buffer, selector + size);
         }
         else
         {
            return CASUAL_ORDER_NO_PLACE;
         }

         return CASUAL_ORDER_SUCCESS;
      }


      int get( char* const buffer, char*& value, long& length)
      {
         constexpr auto size_size = bytes<long>();

         const auto inserter = header::select::inserter( buffer);
         const auto selector = header::select::selector( buffer);

         if( (inserter - selector) < size_size)
         {
            // not even the size is there
            return CASUAL_ORDER_NO_PLACE;
         }

         length = data::select<long>( buffer, selector);

         if( (inserter - selector) < (size_size + length))
         {
            // not even the size is there
            return CASUAL_ORDER_NO_PLACE;
         }

         value = buffer + selector + size_size;

         header::update::selector( buffer, selector + size_size + length);

         return CASUAL_ORDER_SUCCESS;
      }


   } // internal

} // unnamed namespace

char* CasualOrderDescription( int code)
{
   return const_cast<char*>(internal::description( code));
}

int CasualOrderAddPrepare( char* buffer)
{
   constexpr auto bytes = internal::bytes<long>();

   internal::header::update::inserter( buffer, bytes * 3);

   return CASUAL_ORDER_SUCCESS;
}

int CasualOrderAddBool( char* const buffer, bool value)
{
   return internal::add( buffer, value);
}

int CasualOrderAddChar( char* buffer, char value)
{
   return internal::add( buffer, value);
}

int CasualOrderAddShort( char* buffer, short value)
{
   return internal::add( buffer, value);
}

int CasualOrderAddLong( char* buffer, long value)
{
   return internal::add( buffer, value);
}

int CasualOrderAddFloat( char* buffer, float value)
{
   return internal::add( buffer, value);
}

int CasualOrderAddDouble( char* buffer, double value)
{
   return internal::add( buffer, value);
}

int CasualOrderAddString( char* buffer, const char* value)
{
   return internal::add( buffer, value, std::strlen( value) + 1);
}

int CasualOrderAddBinary( char* buffer, const char* value, long size)
{
   return internal::add( buffer, value, size);
}

int CasualOrderGetPrepare( char* buffer)
{
   constexpr auto bytes = internal::bytes<long>();

   internal::header::update::selector( buffer, bytes * 3);

   return CASUAL_ORDER_SUCCESS;
}


#ifdef __bool_true_false_are_defined
int CasualOrderGetBool( char* buffer, bool* value)
{
   return internal::get( buffer, *value);
}

#endif

int CasualOrderGetChar( char* buffer, char* value)
{
   return internal::get( buffer, *value);
}

int CasualOrderGetShort( char* buffer, short* value)
{
   return internal::get( buffer, *value);
}

int CasualOrderGetLong( char* buffer, long* value)
{
   return internal::get( buffer, *value);
}

int CasualOrderGetFloat( char* buffer, float* value)
{
   return internal::get( buffer, *value);
}

int CasualOrderGetDouble( char* buffer, double* value)
{
   return internal::get( buffer, *value);
}

int CasualOrderGetString( char* buffer, char** value)
{
   long size;
   return internal::get( buffer, *value, size);
}

int CasualOrderGetBinary( char* buffer, char** value, long* size)
{
   return internal::get( buffer, *value, *size);
}

int CasualOrderCopyBuffer( char* source, char* target)
{
   const auto target_reserved = internal::header::select::reserved( target);
   const auto source_inserter = internal::header::select::inserter( source);

   if( target_reserved < source_inserter)
   {
      return CASUAL_ORDER_NO_SPACE;
   }

   constexpr auto bytes = internal::bytes<long>();

   // copy inserter and selector (and data) but leave reserved
   std::memcpy( target + bytes, source + bytes, source_inserter - bytes);

   return CASUAL_ORDER_SUCCESS;
}


int CasualOrderUsed( const char* buffer, long* size)
{
   *size = internal::header::select::inserter( buffer);
   return CASUAL_ORDER_SUCCESS;
}


long CasualOrderCreate( char* buffer, long size)
{
   constexpr auto bytes = internal::bytes<long>() * 3;

   if( size < bytes)
   {
      return CASUAL_ORDER_NO_SPACE;
   }

   internal::header::update::reserved( buffer, size);
   internal::header::update::inserter( buffer, bytes);
   internal::header::update::selector( buffer, bytes);

   return CASUAL_ORDER_SUCCESS;
}

long CasualOrderExpand( char* buffer, long size)
{
   internal::header::update::reserved( buffer, size);

   return CASUAL_ORDER_SUCCESS;
}

long CasualOrderReduce( char* buffer, long size)
{
   const auto inserter = internal::header::select::inserter( buffer);

   if( size < inserter)
   {
      return CASUAL_ORDER_NO_SPACE;
   }

   internal::header::update::reserved( buffer, size);

   return CASUAL_ORDER_SUCCESS;
}

long CasualOrderNeeded( char* buffer, long size)
{
   return internal::header::select::inserter( buffer);
}


