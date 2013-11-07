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
         template<typename T,typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
         struct byteorder
         {
            static T encode( const T value) noexcept
            {
               return value;
            }
            static T decode( const T value) noexcept
            {
               return value;
            }
         };

         template<>
         struct byteorder<short>
         {
            static uint32_t encode( const short value) noexcept;
            static short decode( const uint32_t value) noexcept;
         };

         template<>
         struct byteorder<long>
         {
            static uint64_t encode( const long value) noexcept;
            static long decode( const uint64_t value) noexcept;
         };

         template<>
         struct byteorder<float>
         {
            static uint32_t encode( const float value) noexcept;
            static float decode( const uint32_t value) noexcept;
         };

         template<>
         struct byteorder<double>
         {
            static uint64_t encode( const double value) noexcept;
            static double decode( const uint64_t value) noexcept;
         };


      }
   }
}

#endif /* NETWORK_BYTEORDER_H_ */
