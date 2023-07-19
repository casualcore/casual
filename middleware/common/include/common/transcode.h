//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "casual/platform.h"
#include "common/traits.h"
#include "common/range.h"

#include <string>

namespace casual
{
   namespace common::transcode
   {
      namespace base64
      {
         namespace detail
         {
            struct Data 
            {
               void* memory;
               platform::size::type bytes;
            };

            template< typename C> 
            auto data( C& container)
            {
               return Data{ (void*)( container.data()), static_cast< platform::size::type>( container.size())};
            }

            platform::size::type encode( const Data source, Data target);

            platform::size::type decode( const char* first, const char* last, char* dest_first, char* dest_last);
         } // detail

         namespace capacity
         {
            constexpr platform::size::type encoded( platform::size::type bytes) 
            {
               return ( ( bytes + 2) / 3) * 4;
            }
         } // capacity



         template< concepts::binary::like C1, concepts::container::resize C2>
         void encode( C1&& source, C2& target)
         {
            static_assert( sizeof( std::ranges::range_value_t< C1>) == sizeof( std::ranges::range_value_t< C2>), "not the same value type size");

            target.resize( capacity::encoded( source.size()));

            target.resize( detail::encode( detail::data( source), detail::data( target)));
         }

         //! @return Base64-encoded binary data of @p container
         //!
         //! @throw exception::Casual on failure
         template< concepts::binary::like C>
         std::string encode( C&& container)
         {
            std::string result;
            encode( container, result);
            return result;
         }

         //! @return Base64-encoded binary data of [first, last)
         //!
         //! @pre @p Iter has to be a random access iterator
         //!
         //! @throw exception::Casual on failure
         template< concepts::binary::iterator Iter>
         std::string encode( Iter first, Iter last)
         {
            return encode( range::make( first, last));
         }

         //! @return Base64-decoded binary data
         //!
         //! @throw exception::Casual on failure
         platform::binary::type decode( const std::string& value);

         // TODO performance: make it possible to decode to fixed memory
         //  `b64_pton` seems to need additional space during decode, hence it's
         //  not symmetric. Roll our own?


         //!
         //! decode Base64 to a binary representation
         //! @attention [first, last) needs to be bigger than the [first, result) (by some bytes...)
         //!
         template< concepts::binary::iterator Iter>
         auto decode( std::string_view source, Iter first, Iter last)
         {
            auto cast_source = []( auto&& i){ return reinterpret_cast< const char*>( &(*i));};
            auto cast_target = []( auto&& i){ return reinterpret_cast< char*>( &(*i));};
            auto size = detail::decode(
               cast_source( std::begin( source)), cast_source( std::end( source)),
               cast_target( first), cast_target( last));

            return first + size;
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
         std::string encode( const std::string& value);

         //! @param value The UTF-8 encoded string
         //!
         //! @return String encoded in local default codeset
         //!
         //! @throw exception::limit::Memory on resource failures
         //! @throw exception::system::invalid::Argument for bad input
         //! @throw exception::Casual on other failures
         std::string decode( const std::string& value);

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
         std::string encode( const std::string& value, std::string_view codeset);

         //! @param value The UTF-8 encoded string
         //! @param codeset Encoding for result
         //!
         //! @return String encoded in provided codeset
         //!
         //! @throw exception::limit::Memory on resource failures
         //! @throw exception::system::invalid::Argument for bad input
         //! @throw exception::Casual on other failures
         std::string decode( const std::string& value, std::string_view codeset);

      } // utf8

      namespace hex
      {
         namespace detail
         {
            template< typename Input, typename Out>
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
                  *out++ = hex( ( 0xf0 & *first) >> 4);
                  *out++ = hex( 0x0f & *first);
               }
            }

            template< typename Input, typename Out>
            void decode( Input first, Input last, Out out)
            {
               assert( ( last - first) % 2 == 0);

               auto hex = []( auto value)
               {
                  if( value >= 87)
                     return value - 87;
                  return value - 48;
               };

               for( ; first != last; ++out)
               {
                  *out = ( 0x0f & hex( *first++)) << 4;
                  *out += 0x0f & hex( *first++);
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



