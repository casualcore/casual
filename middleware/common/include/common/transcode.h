//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "casual/platform.h"
#include "common/traits.h"
#include "common/range.h"
#include "common/binary/span.h"

#include <string>

namespace casual
{
   namespace common::transcode
   {
      namespace base64
      {
         namespace detail
         {
            platform::size::type encode( std::span< const std::byte> source, std::span< std::byte> target);
            platform::size::type decode( std::span< const std::byte> source, std::span< std::byte> target);
         } // detail

         //! @note only supports byte sized source (so far) (target is naturally byte sized)
         template< concepts::container::bytes S, concepts::container::bytes T>
         void encode( const S& source, T& target)
         {
            target.resize( ( std::size( source) + 2) / 3 * 4);
            target.resize( detail::encode( std::as_bytes( std::span{ source}), std::as_writable_bytes( std::span{ target})));
         }

         //! @note only supports byte sized target (so far) (source is naturally byte sized)
         template< concepts::container::bytes S, concepts::container::bytes T>
         void decode( const S& source, T& target)
         {
            target.resize( ( std::size( source) / 4) * 3);
            target.resize( detail::decode( std::as_bytes( std::span{ source}), std::as_writable_bytes( std::span{ target})));
         }

         //! @return Base64-encoded data
         std::string encode( const auto& value)
         {
            std::string result;
            encode( value, result);
            return result;
         }

         //! @return Base64-decoded data
         //!
         //! @throw exception::Casual on failure
         platform::binary::type decode( const auto& value)
         {
            platform::binary::type result;
            decode( value, result);
            return result;
         }
      } // base64

      namespace utf8
      {
         //! @param value String encoded in local default codeset
         //!
         //! @return UTF-8-encoded string
         //!
         //! @throw exception::limit::Memory on resource failures
         //! @throw exception::system::invalid::Argument for bad input
         //! @throw exception::Casual on other failures
         std::u8string encode( std::string_view value);

         //! @param value The UTF-8 encoded string
         //!
         //! @return String encoded in local default codeset
         //!
         //! @throw exception::limit::Memory on resource failures
         //! @throw exception::system::invalid::Argument for bad input
         //! @throw exception::Casual on other failures
         std::string decode( std::u8string_view value);

         //! @param codeset String-encoding
         //!
         //! @return Whether the provided codeset exist in the system
         //!
         //! @throw exception::limit::Memory on resource failures
         //! @throw exception::Casual on other failures
         bool exist( std::string_view codeset);

         //! @param value String encoded in provided codeset
         //! @param codeset String-encoding
         //!
         //! @return UTF-8-encoded string
         //!
         //! @throw exception::limit::Memory on resource failures
         //! @throw exception::system::invalid::Argument for bad input
         //! @throw exception::Casual on other failures
         std::u8string encode( std::string_view value, std::string_view codeset);

         //! @param value The UTF-8 encoded string
         //! @param codeset Encoding for result
         //!
         //! @return String encoded in provided codeset
         //!
         //! @throw exception::limit::Memory on resource failures
         //! @throw exception::system::invalid::Argument for bad input
         //! @throw exception::Casual on other failures
         std::string decode( std::u8string_view value, std::string_view codeset);

         inline std::u8string_view cast( std::string_view value) { return { reinterpret_cast< decltype( cast( value))::const_pointer>(value.data()), value.size()};}
         inline std::string_view cast( std::u8string_view value) { return { reinterpret_cast< decltype( cast( value))::const_pointer>(value.data()), value.size()};}

         namespace string
         {
            //! @see utf8::encode
            std::string encode( std::string_view value);
            //! @see utf8::decode
            std::string decode( std::string_view value);

            //! @see utf8::encode
            std::string encode( std::string_view value, std::string_view codeset);
            //! @see utf8::decode
            std::string decode( std::string_view value, std::string_view codeset);
         } // string

      } // utf8

      namespace hex
      {
         namespace detail
         {
            template< concepts::binary::iterator Input, typename Out>
            void encode( Input first, Input last, Out out)
            {
               auto hex = []( auto value)
               {
                  if( value < 10)
                     return value + 48;
                  return value + 87;
               };

               for( ; first != last; ++first)
               {
                  const auto value = std::to_integer< std::int8_t>( *first);
                  *out++ = hex( ( 0xf0 & value) >> 4);
                  *out++ = hex( 0x0f & value);
               }
            }

            template< typename Input, concepts::binary::iterator Out>
            void decode( Input first, Input last, Out out)
            {
               assert( std::distance( first, last) % 2 == 0);

               auto hex = []( auto value)
               {
                  if( value >= 87)
                     return value - 87;
                  return value - 48;
               };

               for( ; first != last; ++out)
               {
                  auto value = ( 0x0f & hex( *first++)) << 4;
                  value += 0x0f & hex( *first++);

                  *out = static_cast< std::byte>( value);
               }
            }

         } // detail


         //! encode binary sequence [first, last) to hex-representation
         //!
         //! @param first start of binary
         //! @param last end of binary (exclusive)
         //! @return hex-encoded string of [first, last)
         template< concepts::binary::iterator Iter>
         std::string encode( Iter first, Iter last)
         {
            std::string result( std::distance( first, last) * 2, 0);
            detail::encode( first, last, std::begin( result));
            return result;
         }

         template< concepts::binary::iterator Iter>
         std::ostream& encode( std::ostream& out, Iter first, Iter last)
         {
            detail::encode( first, last, std::ostream_iterator< char>( out));
            return out;
         }

         template< concepts::binary::like R>
         std::ostream& encode( std::ostream& out, const R& range)
         {
            return encode( out, std::begin( range), std::end( range));
         }

         //! encode binary @p container to hex-representation
         //!
         //! @param container binary representation
         //! @return hex-encoded string of @p container
         template< concepts::binary::like C>
         std::string encode( const C& container)
         {
            return encode( std::begin( container), std::end( container));
         }

         //! decode hex-string to a binary representation
         //!
         //! @param value hex-string
         //! @return binary representation of @p value
         inline platform::binary::type decode( std::string_view value)
         {
            platform::binary::type result( value.size() / 2);
            detail::decode( std::begin( value), std::end( value), std::begin( result));
            return result;
         }

         template< concepts::binary::iterator Iter>
         void decode( std::string_view source, Iter first, Iter last)
         {
            assert( range::size( source) <= ( std::distance( first, last)  * 2) + 1);
            detail::decode( std::begin( source), std::end( source), first);
         }

         //! decode hex-string to a binary representation
         //!
         //! @return binary representation of @p value
         template< concepts::binary::like Target>
         void decode( std::string_view source, Target&& target)
         {
            decode( source, std::begin( target), std::end( target));
         }

         namespace stream
         {
            namespace detail
            {
               template< typename T>
               struct Proxy
               {
                  friend std::ostream& operator << ( std::ostream& out, const Proxy& proxy)
                  {
                     return hex::encode( out, *proxy.value);
                  }

                  const T* value;
               };
            } // detail

            template< typename T>
            auto wrapper( const T& value)
            {
               return detail::Proxy< T>{ &value};
            }
         } // stream

      } // hex
   } // common::transcode
} // casual



