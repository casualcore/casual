//
// network_byteorder.h
//
//  Created on: 7 nov 2013
//      Author: Kristone
//

#ifndef CASUAL_COMMON_NETWORK_BYTEORDER_H_
#define CASUAL_COMMON_NETWORK_BYTEORDER_H_

#include <cstdint>

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

               template<typename type>
               struct transcode;

               template<>
               struct transcode<bool>
               {
                  static std::uint8_t encode( bool value) noexcept;
                  static bool decode( std::uint8_t value) noexcept;
               };

               template<>
               struct transcode<char>
               {
                  static std::uint8_t encode( char value) noexcept;
                  static char decode( std::uint8_t value) noexcept;
               };


               template<>
               struct transcode<short>
               {
                  static std::uint16_t encode( short value) noexcept;
                  static short decode( std::uint16_t value) noexcept;
               };

               template<>
               struct transcode<int>
               {
                  static std::uint32_t encode( int value) noexcept;
                  static int decode( std::uint32_t value) noexcept;
               };

               template<>
               struct transcode<long>
               {
                  static std::uint64_t encode( long value) noexcept;
                  static long decode( std::uint64_t value) noexcept;
               };

               template<>
               struct transcode<long long>
               {
                  static std::uint64_t encode( long long value) noexcept;
                  static long long decode( std::uint64_t value) noexcept;
               };


               template<>
               struct transcode<float>
               {
                  static std::uint32_t encode( float value) noexcept;
                  static float decode( std::uint32_t value) noexcept;
               };

               template<>
               struct transcode<double>
               {
                  static std::uint64_t encode( double value) noexcept;
                  static double decode( std::uint64_t value) noexcept;
               };

            } // detail


            template< typename T>
            inline auto encode( const T value) noexcept
            {
               return detail::transcode<T>::encode( value);
            }

            template< typename R>
            inline auto decode( const decltype(encode(R{})) value) noexcept
            {
               return detail::transcode<R>::decode( value);
            }


            template<typename T>
            using type = decltype( encode( T{}));

            template< typename T>
            constexpr std::size_t bytes() noexcept
            {
               return sizeof( decltype( encode( T{})));
            }


            namespace size
            {
               using S = long;
               using type = S;

               template< typename T>
               inline auto encode( const T value) noexcept
               {
                  return detail::transcode<S>::encode( static_cast<S>(value));
               }

               template< typename R>
               inline auto decode( const decltype(encode(R{})) value) noexcept
               {
                  return static_cast<R>( detail::transcode<S>::decode( value));
               }
            } // size


         } // byteorder

      } // network
   }
}

#endif /* CASUAL_COMMON_NETWORK_BYTEORDER_H_ */
