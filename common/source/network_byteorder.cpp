//
// network_byteorder.cpp
//
//  Created on: 7 nov 2013
//      Author: Kristone
//

#include "common/network_byteorder.h"

#include <unistd.h>

#if defined (__linux__)
   #include <endian.h>
   #include <byteswap.h>
#elif defined (__APPLE__)
   #include <netinet/in.h>
   #include <libkern/OSByteOrder.h>
#elif defined (__OpenBSD__)
   #include <sys/types.h>
#elif defined (WIN32)
   #include <stdint.h>
   #define BYTE_ORDER LITTLE_ENDIAN
#elif defined (__vxworks)
   #include <netinet/in.h>
#else
   #error "Unknown environment"
#endif


#if defined (BYTE_ORDER)
   #if BYTE_ORDER == LITTLE_ENDIAN
      #define BE16TOH(x) __bswap_16(x)
      #define BE32TOH(x) __bswap_32(x)
      #define BE64TOH(x) __bswap_64(x)
      #define HTOBE16(x) __bswap_16(x)
      #define HTOBE32(x) __bswap_32(x)
      #define HTOBE64(x) __bswap_64(x)
      #define LE16TOH(x) (x)
      #define LE32TOH(x) (x)
      #define LE64TOH(x) (x)
      #define HTOLE16(x) (x)
      #define HTOLE32(x) (x)
      #define HTOLE64(x) (x)
   #elif BYTE_ORDER == BIG_ENDIAN
      #define BE16TOH(x) (x)
      #define BE32TOH(x) (x)
      #define BE64TOH(x) (x)
      #define HTOBE16(x) (x)
      #define HTOBE32(x) (x)
      #define HTOBE64(x) (x)
      #define LE16TOH(x) __bswap_16(x)
      #define LE32TOH(x) __bswap_32(x)
      #define LE64TOH(x) __bswap_64(x)
      #define HTOLE16(x) __bswap_16(x)
      #define HTOLE32(x) __bswap_32(x)
      #define HTOLE64(x) __bswap_64(x)
   #else
      #error "Undefined host byte order"
   #endif
#elif defined (__vxworks) && defined (_BYTE_ORDER)
   #if _BYTE_ORDER == _LITTLE_ENDIAN
      #define BE16TOH(x) __bswap_16(x)
      #define BE32TOH(x) __bswap_32(x)
      #define BE64TOH(x) __bswap_64(x)
      #define HTOBE16(x) __bswap_16(x)
      #define HTOBE32(x) __bswap_32(x)
      #define HTOBE64(x) __bswap_64(x)
      #define LE16TOH(x) (x)
      #define LE32TOH(x) (x)
      #define LE64TOH(x) (x)
      #define HTOLE16(x) (x)
      #define HTOLE32(x) (x)
      #define HTOLE64(x) (x)
   #elif _BYTE_ORDER == _BIG_ENDIAN
      #define __bswap_32(x)    ((((x) & 0x000000ff) << 24) | (((x) & 0x0000ff00) <<  8) | (((x) & 0x00ff0000) >>  8) | (((x) & 0xff000000) >> 24))
      #define __bswap_16(x)    ((((x) & 0x00ff) << 8) | (((x) & 0xff00) >> 8))
      #define BE16TOH(x) (x)
      #define BE32TOH(x) (x)
      #define BE64TOH(x) (x)
      #define HTOBE16(x) (x)
      #define HTOBE32(x) (x)
      #define HTOBE64(x) (x)
      #define LE16TOH(x) __bswap_16(x)
      #define LE32TOH(x) __bswap_32(x)
      #define LE64TOH(x) __bswap_64(x)
      #define HTOLE16(x) __bswap_16(x)
      #define HTOLE32(x) __bswap_32(x)
      #define HTOLE64(x) __bswap_64(x)
   #else
      #error "Undefined host byte order"
   #endif
#else
   #error "Undefined host byte order"
#endif

#if defined (WIN32)

   uint16_t __bswap_16 (uint16_t x);
   uint32_t __bswap_32 (uint32_t x);
   uint64_t __bswap_64 (uint64_t x);

#elif defined (__OpenBSD__)

   #define __bswap_16(x) swap16(x)
   #define __bswap_32(x) swap32(x)
   #define __bswap_64(x) swap64(x)

#elif defined (__APPLE__)

   #define __bswap_16(x) OSSwapInt16(x)
   #define __bswap_32(x) OSSwapInt32(x)
   #define __bswap_64(x) OSSwapInt64(x)

#elif defined (__linux__)

#else
   #error "Unknown Environment"
#endif



namespace casual
{
   namespace common
   {
      namespace network
      {

         uint8_t byteorder< bool>::encode( const bool value) noexcept
         {
            return value;
         }
         bool byteorder< bool>::decode( const uint8_t value) noexcept
         {
            return value;
         }

         uint8_t byteorder< char>::encode( const char value) noexcept
         {
            return value;
         }
         char byteorder< char>::decode( const uint8_t value) noexcept
         {
            return value;
         }

         //
         // short is 16 bit on every 32/64 bit systems except SILP64 systems
         //
         uint16_t byteorder< short>::encode( const short value) noexcept
         {
            return htobe16(value);
         }
         short byteorder< short>::decode( const uint16_t value) noexcept
         {
            return be16toh(value);
         }

         uint64_t byteorder< long>::encode( const long value) noexcept
         {
            return htobe64( value);
         }
         long byteorder< long>::decode( const uint64_t value) noexcept
         {
            return be64toh( value);
         }

         //
         // we assume IEEE 754 and that float is 32 bits
         //
         uint32_t byteorder< float>::encode( const float value) noexcept
         {
            return htobe32( *reinterpret_cast<const uint32_t*>(&value));
         }
         float byteorder< float>::decode( const uint32_t value) noexcept
         {
            const uint32_t result = be32toh(value);
            return *reinterpret_cast< const float*>( &result);
         }

         //
         // we assume IEEE 754 and that double is 64 bits
         //
         uint64_t byteorder< double>::encode( const double value) noexcept
         {
            return htobe64(*reinterpret_cast<const uint64_t*>(&value));
         }
         double byteorder< double>::decode( const uint64_t value) noexcept
         {
            const uint64_t result = be64toh(value);
            return *reinterpret_cast< const double*>( &result);
         }







         uint8_t byteorder< uint8_t>::encode( const uint8_t value) noexcept
         {
            return value;
         }
         uint8_t byteorder< uint8_t>::decode( const uint8_t value) noexcept
         {
            return value;
         }

         uint16_t byteorder< uint16_t>::encode( const uint16_t value) noexcept
         {
            return htobe16(value);
         }
         uint16_t byteorder< uint16_t>::decode( const uint16_t value) noexcept
         {
            return be16toh( value);
         }


         uint32_t byteorder< uint32_t>::encode( const uint32_t value) noexcept
         {
            return htobe32(value);
         }
         uint32_t byteorder< uint32_t>::decode( const uint32_t value) noexcept
         {
            return be32toh( value);
         }

         uint64_t byteorder< uint64_t>::encode( const uint64_t value) noexcept
         {
            return htobe64(value);
         }
         uint64_t byteorder< uint64_t>::decode( const uint64_t value) noexcept
         {
            return be64toh( value);
         }


      }
   }

}
