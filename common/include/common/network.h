//
// network.h
//
//  Created on: 3 nov 2013
//      Author: Kristone
//

#ifndef NETWORK_H_
#define NETWORK_H_

#include <netinet/in.h>

namespace casual
{
   namespace common
   {
      namespace network
      {

         //
         // This is a fairly naive implementation and not that platform
         // independent either, but it will do so far
         //
         // TODO: make specializations for various unsigned types so that not
         // the default one is chosen
         //

         template< typename T>
         struct transcoder
         {
            static T encode( const T value)
            {
               return value;
            }
            static T decode( const T value)
            {
               return value;
            }
         };

         template< >
         struct transcoder< short>
         {
            static uint32_t encode( const short value)
            {
               return htobe32(value);
            }
            static short decode( const uint32_t value)
            {
               return be32toh(value);
            }
         };

         template< >
         struct transcoder< long>
         {
            static uint64_t encode( const long value)
            {
               return htobe64(value);
            }
            static long decode( const uint64_t value)
            {
               return be64toh(value);
            }
         };

         //
         // TODO: we assume IEEE 754 and that float is 32 bits
         //
         template< >
         struct transcoder< float>
         {
            static uint32_t encode( const float value)
            {
               return htobe32(*reinterpret_cast<const uint32_t*>(&value));
            }
            static float decode( const uint32_t value)
            {
               const uint32_t result be32toh(value);
               return *reinterpret_cast< const float*>( &result);
            }
         };

         //
         // TODO: we assume IEEE 754 and that double is 64 bits
         //
         template< >
         struct transcoder< double>
         {
            static uint64_t encode( const double value)
            {
               return htobe64(*reinterpret_cast<const uint64_t*>(&value));
            }
            static double decode( const uint64_t value)
            {
               const uint64_t result be64toh(value);
               return *reinterpret_cast< const double*>( &result);
            }
         };


      }
   }
}

#endif /* NETWORK_H_ */
