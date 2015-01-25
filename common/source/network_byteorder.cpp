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

         namespace byteorder
         {
            namespace detail
            {

               std::uint16_t transcode< 2, false, false>::encode( std::uint16_t value) noexcept
               {
                  return HTOBE16(value);
               }

               std::uint16_t transcode< 2, false, false>::decode( std::uint16_t value) noexcept
               {
                  return BE16TOH(value);
               }


               std::uint32_t transcode< 4, false, false>::encode( std::uint32_t value) noexcept
               {
                  return HTOBE32(value);
               }

               std::uint32_t transcode< 4, false, false>::decode( std::uint32_t value) noexcept
               {
                  return BE32TOH( value);
               }


               std::uint64_t transcode< 8, false, false>::encode( std::uint64_t value) noexcept
               {
                  return HTOBE64( value);
               }

               std::uint64_t transcode< 8, false, false>::decode( std::uint64_t value) noexcept
               {
                  return BE64TOH( value);
               }



               std::uint64_t transcode< 8, true, true>::encode( double value) noexcept
               {
                  const auto aliasing = reinterpret_cast<const char*>(&value);
                  return HTOBE64(*reinterpret_cast<const uint64_t*>(aliasing));
               }

               double transcode< 8, true, true>::decode( std::uint64_t value) noexcept
               {
                  const auto host = BE64TOH(value);
                  const auto aliasing = reinterpret_cast<const char*>(&host);
                  return *reinterpret_cast< const double*>( aliasing);
               }


               std::uint32_t transcode< 4, true, true>::encode( float value) noexcept
               {
                  const auto aliasing = reinterpret_cast<const char*>(&value);
                  return HTOBE32( *reinterpret_cast< const std::uint32_t*>(aliasing));
               }


               float transcode< 4, true, true>::decode( std::uint32_t value) noexcept
               {
                  const auto host = BE32TOH( value);
                  const auto aliasing = reinterpret_cast<const char*>( &host);
                  return *reinterpret_cast< const float*>( aliasing);
               }

            } // detail
         } // byteorder
      } // nework
   } // common
} // casual
