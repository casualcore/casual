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

                  template< typename T>
                  using is_network_array = std::integral_constant< bool,
                        std::is_array< T>::value >; //&&
                        //std::is_array< typename std::decay< T>::type>::value >; //&&
                        //sizeof( typename std::remove_all_extents<  typename std::decay< T>::type>::type) == 1>;

               } // detail

               struct Policy
               {
                  template< typename T>
                  static constexpr typename std::enable_if< ! detail::is_network_array< T>::value, std::size_t>::type size( T&)
                  {
                     return sizeof( common::network::byteorder::type< typename std::decay< T>::type>);
                  }

                  template< typename T>
                  static constexpr typename std::enable_if< detail::is_network_array< T>::value, std::size_t>::type size( T&)
                  {
                     return sizeof( T);
                  }

                  template< typename Iter, typename T>
                  static typename std::enable_if< ! detail::is_network_array< T>::value>::type write( Iter buffer, T& value)
                  {
                     auto net_value = common::network::byteorder::encode( value);
                     memcpy( buffer, &net_value, size( value));
                  }

                  template< typename Iter, typename T>
                  static typename std::enable_if< detail::is_network_array< T>::value>::type write( Iter buffer, T& value)
                  {
                     memcpy( buffer, &value, size( value));
                  }

                  template< typename Iter, typename T>
                  static typename std::enable_if< ! detail::is_network_array< T>::value>::type read( T& value, Iter buffer)
                  {
                     common::network::byteorder::type< T> net_value;
                     memcpy( &net_value, buffer, size( value));
                     value = common::network::byteorder::decode< T>( net_value);
                  }

                  template< typename Iter, typename T>
                  static typename std::enable_if< detail::is_network_array< T>::value>::type read( T& value, Iter buffer)
                  {
                     memcpy( &value, buffer, size( value));
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
