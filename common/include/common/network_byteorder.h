//
// network_byteorder.h
//
//  Created on: 7 nov 2013
//      Author: Kristone
//

#ifndef NETWORK_BYTEORDER_H_
#define NETWORK_BYTEORDER_H_

#include <type_traits>
#include <cstdint>

namespace casual
{
   namespace common
   {
      namespace network
      {

         template< typename T, typename = typename std::enable_if< std::is_arithmetic< T>::value, T>::type>
         struct byteorder
         {
         };

         template<typename T>
         using type = decltype(byteorder<T>::encode(0));

         template< typename T>
         constexpr std::size_t bytes() noexcept
         {
            return sizeof(type<T>);
         }


         template< >
         struct byteorder< bool>
         {
            static uint8_t encode( bool value) noexcept;
            static bool decode( uint8_t value) noexcept;
         };

         template< >
         struct byteorder< char>
         {
            static uint8_t encode( char value) noexcept;
            static char decode( uint8_t value) noexcept;
         };

         template< >
         struct byteorder< short>
         {
            static uint16_t encode( short value) noexcept;
            static short decode( uint16_t value) noexcept;
         };

         template< >
         struct byteorder< long>
         {
            static uint64_t encode( long value) noexcept;
            static long decode( uint64_t value) noexcept;
         };

         template< >
         struct byteorder< float>
         {
            static uint32_t encode( float value) noexcept;
            static float decode( uint32_t value) noexcept;
         };

         template< >
         struct byteorder< double>
         {
            static uint64_t encode( double value) noexcept;
            static double decode( uint64_t value) noexcept;
         };

         //
         // Fixed width unsigned integer types
         //
         // TODO: Shall we have these ?
         //

         template< >
         struct byteorder< uint8_t>
         {
            static uint8_t encode( uint8_t value) noexcept;
            static uint8_t decode( uint8_t value) noexcept;
         };

         template< >
         struct byteorder< uint16_t>
         {
            static uint16_t encode( uint16_t value) noexcept;
            static uint16_t decode( uint16_t value) noexcept;
         };

         template< >
         struct byteorder< uint32_t>
         {
            static uint32_t encode( uint32_t value) noexcept;
            static uint32_t decode( uint32_t value) noexcept;
         };

         template< >
         struct byteorder< uint64_t>
         {
            static uint64_t encode( uint64_t value) noexcept;
            static uint64_t decode( uint64_t value) noexcept;
         };

      }
   }
}

#endif /* NETWORK_BYTEORDER_H_ */
