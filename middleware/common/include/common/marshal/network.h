//!
//! casual
//!

#ifndef CASUAL_COMMON_MARSHAL_NETWORK_H_
#define CASUAL_COMMON_MARSHAL_NETWORK_H_

#include "common/marshal/binary.h"
#include "common/network/byteorder.h"

#include "common/cast.h"

namespace casual
{
   namespace common
   {
      namespace marshal
      {
         namespace binary
         {
            namespace network
            {
               using size_type = platform::size::type;
               
               namespace detail
               {

                  //
                  // Helper to make sure we only transport byte-arrays
                  //
                  template< typename T>
                  using is_network_array = std::integral_constant< bool,
                        ( std::is_array< typename std::remove_reference< T>::type>::value
                          && sizeof( typename std::remove_all_extents< typename std::remove_reference< T>::type>::type) == 1)
                          || traits::container::is_array< T>::value>;



                  template< typename T>
                  constexpr auto cast( T value) -> typename std::enable_if< ! std::is_enum< T>::value, T>::type
                  {
                     return value;
                  }


                  template< typename T>
                  constexpr auto cast( T value) -> typename std::enable_if< std::is_enum< T>::value, decltype( cast::underlying( value))>::type
                  {
                     return common::cast::underlying( value);
                  }

               } // detail

               struct Policy
               {

                  template< typename T>
                  static typename std::enable_if< ! detail::is_network_array< T>::value>::type
                  write( const T& value, platform::binary::type& buffer)
                  {
                     auto net_value = common::network::byteorder::encode( detail::cast( value));
                     memory::append( net_value, buffer);
                  }

                  template< typename T>
                  static typename std::enable_if< detail::is_network_array< T>::value>::type
                  write( const T& value, platform::binary::type& buffer)
                  {
                     memory::append( value, buffer);
                  }

                  template<typename T>
                  static void write_size( const T& size, platform::binary::type& buffer)
                  {
                     const auto encoded = common::network::byteorder::size::encode( size);
                     memory::append( encoded, buffer);
                  }


                  template< typename T>
                  static std::enable_if_t< ! detail::is_network_array< T>::value, size_type>
                  read( const platform::binary::type& buffer, size_type offset, T& value)
                  {
                     using value_type = decltype( detail::cast( value));
                     common::network::byteorder::type< value_type> net_value;
                     offset = memory::copy( buffer, offset, net_value);
                     value = static_cast< T>( common::network::byteorder::decode< value_type>( net_value));

                     return offset;
                  }

                  template< typename T>
                  static std::enable_if_t< detail::is_network_array< T>::value, size_type>
                  read( const platform::binary::type& buffer, size_type offset, T& value)
                  {
                     return memory::copy( buffer, offset, value);
                  }

                  template<typename T>
                  static size_type read_size( const platform::binary::type& buffer, size_type offset, T& value)
                  {
                     decltype(common::network::byteorder::size::encode( value)) encoded;
                     offset = memory::copy( buffer, offset, encoded);
                     value = common::network::byteorder::size::decode<T>( encoded);
                     return offset;
                  }


               };

               using Input = basic_input< Policy>;

               using Output = basic_output< Policy>;

               namespace create
               {
                  struct Output
                  {
                     network::Output operator () ( platform::binary::type& buffer) const
                     {
                        return network::Output{ buffer};
                     }
                  };

                  struct Input
                  {
                     network::Input operator () ( platform::binary::type& buffer) const
                     {
                        return network::Input{ buffer};
                     }
                  };

               } // create

            } // network

         } // binary

         template<>
         struct is_network_normalizing< binary::network::Input>: std::true_type {};

         template<>
         struct is_network_normalizing< binary::network::Output>: std::true_type {};

      } // marshal
   } // common
} // casual

#endif // CASUAL_COMMON_MARSHAL_NETWORK_H_
