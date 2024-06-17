//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "common/serialize/native/binary.h"
#include "common/network/byteorder.h"
#include "common/traits.h"

namespace casual
{
   namespace common::serialize
   {
      namespace native::binary::network
      {

         using size_type = platform::size::type;
         
         namespace detail
         {
            template< typename T>
            concept network_array = concepts::binary::like< T>;

            template< typename T>
            concept network_value = ! network_array< T>;

            template< typename T>
            constexpr auto cast( T value)
            {
               if constexpr( std::is_enum_v< T>)
                  return std::to_underlying( value);
               else
                  return value;
            }
         } // detail

         struct Policy
         {
            constexpr static auto archive_properties() { return common::serialize::archive::Property::order | common::serialize::archive::Property::network;}

            template< detail::network_value T>
            static void write( const T& value, platform::binary::type& buffer) 
            {
               auto net_value = common::network::byteorder::encode( detail::cast( value));
               memory::append( net_value, buffer);
            }

            template< detail::network_array T>
            static auto write( const T& value, platform::binary::type& buffer)
            {
               memory::append( value, buffer);
            }

            template<typename T>
            static void write_size( const T& size, platform::binary::type& buffer)
            {
               const auto encoded = common::network::byteorder::size::encode( size);
               memory::append( encoded, buffer);
            }


            template< detail::network_value T>
            static platform::size::type read( const platform::binary::type& buffer, size_type offset, T& value)
            {
               using value_type = decltype( detail::cast( value));
               common::network::byteorder::type< value_type> net_value;
               offset = memory::copy( buffer, offset, net_value);
               value = static_cast< T>( common::network::byteorder::decode< value_type>( net_value));

               return offset;
            }

            template< detail::network_array T>
            static platform::size::type read( const platform::binary::type& buffer, size_type offset, T& value)
            {
               return memory::copy( buffer, offset, value);
            }

            template<typename T>
            static platform::size::type read_size( const platform::binary::type& buffer, size_type offset, T& value)
            {
               decltype(common::network::byteorder::size::encode( value)) encoded;
               offset = memory::copy( buffer, offset, encoded);
               value = common::network::byteorder::size::decode<T>( encoded);
               return offset;
            }


         };

         using Reader = basic_reader< Policy>;
         static_assert( archive::is::network::normalizing< Reader>);

         using Writer = basic_writer< Policy>;
         static_assert( archive::is::network::normalizing< Writer>);

         namespace create
         {
            struct Writer
            {
               inline auto operator () () const
               {
                  return network::Writer{};
               }
            };

            struct Reader
            {
               inline auto operator () ( platform::binary::type& buffer) const
               {
                  return network::Reader{ buffer};
               }
            };

         } // create
      } // native::binary::network

      namespace native::create
      {
         template<>
         struct reverse< binary::network::create::Writer> { using type = binary::network::create::Reader;};

         template<>
         struct reverse< binary::network::create::Reader> { using type = binary::network::create::Writer;};

      } // native::create
      
   } // common::serialize
} // casual


