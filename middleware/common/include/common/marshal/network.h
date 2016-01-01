//!
//! network.h
//!
//! Created on: Sep 27, 2015
//!     Author: Lazan
//!

#ifndef CASUAL_COMMON_MARSHAL_NETWORK_H_
#define CASUAL_COMMON_MARSHAL_NETWORK_H_

#include "common/marshal/binary.h"

#include "common/network/byteorder.h"

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
               namespace detail
               {

                  //
                  // Helper to make sure we only transport byte-arrays
                  //
                  template< typename T>
                  using is_network_array = std::integral_constant< bool,
                        std::is_array< typename std::remove_reference< T>::type>::value
                        && sizeof( typename std::remove_all_extents< typename std::remove_reference< T>::type>::type) == 1>;

               } // detail

               struct Policy
               {

                  template< typename T>
                  static typename std::enable_if< ! detail::is_network_array< T>::value>::type
                  write( const T& value, platform::binary_type& buffer)
                  {
                     auto net_value = common::network::byteorder::encode( value);
                     memory::append( net_value, buffer);
                  }

                  template< typename T>
                  static typename std::enable_if< detail::is_network_array< T>::value>::type
                  write( const T& value, platform::binary_type& buffer)
                  {
                     memory::append( value, buffer);
                  }


                  template< typename T>
                  static typename std::enable_if< ! detail::is_network_array< T>::value, std::size_t>::type
                  read( const platform::binary_type& buffer, std::size_t offset, T& value)
                  {
                     common::network::byteorder::type< T> net_value;
                     offset = memory::copy( buffer, offset, net_value);
                     value = common::network::byteorder::decode< T>( net_value);

                     return offset;
                  }

                  template< typename T>
                  static typename std::enable_if< detail::is_network_array< T>::value, std::size_t>::type
                  read( const platform::binary_type& buffer, std::size_t offset, T& value)
                  {
                     return memory::copy( buffer, offset, value);
                  }
               };

               using Input = basic_input< Policy>;

               using Output = basic_output< Policy>;

               namespace create
               {
                  struct Output
                  {
                     network::Output operator () ( platform::binary_type& buffer) const
                     {
                        return network::Output{ buffer};
                     }
                  };

                  struct Input
                  {
                     network::Input operator () ( platform::binary_type& buffer) const
                     {
                        return network::Input{ buffer};
                     }
                  };

               } // create

            } // network

         } // binary

      } // marshal
   } // common
} // casual

#endif // CASUAL_COMMON_MARSHAL_NETWORK_H_
