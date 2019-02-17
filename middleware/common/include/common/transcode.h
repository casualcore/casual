//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "common/platform.h"
#include "common/traits.h"
#include "common/range.h"

#include <string>

namespace casual
{
   namespace common
   {
      namespace transcode
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
                  return ( ( bytes + 2) / 3) * 4 + 1;
               }
            } // size



            template< typename C1, typename C2>
            auto encode( C1&& source, C2& target) -> std::enable_if_t< traits::has::resize< decltype( target)>::value>
            {
               static_assert( sizeof( traits::iterable::value_t< C1>) == sizeof( traits::iterable::value_t< C2>), "not the same value type size");

               target.resize( capacity::encoded( source.size()));

               target.resize( detail::encode( detail::data( source), detail::data( target)));
            }

            //!
            //! @return Base64-encoded binary data of @p container
            //!
            //! @throw exception::Casual on failure
            //!
            template< typename C>
            std::string encode( C&& container)
            {
               std::string result;
               encode( container, result);
               return result;
            }

            //!
            //! @return Base64-encoded binary data of [first, last)
            //!
            //! @pre @p Iter has to be a random access iterator
            //!
            //! @throw exception::Casual on failure
            //!
            template< typename Iter>
            auto encode( Iter first, Iter last) -> 
               std::enable_if_t< traits::is::iterator< Iter>::value, std::string>
            {
               return encode( range::make( first, last));
            }

            //!
            //! @return Base64-decoded binary data
            //!
            //! @throw exception::Casual on failure
            //!
            platform::binary::type decode( const std::string& value);

            //!
            //! decode Base64 to a binary representation
            //!
            //!
            template< typename Source, typename Iter>
            std::enable_if_t<
               traits::is::string::like< Source>::value
               && traits::is::binary::iterator< Iter>::value,
               Iter >
            decode( Source&& source, Iter first, Iter last)
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
            //!
            //! @param value String encoded in local default codeset
            //!
            //! @return UTF-8-encoded string
            //!
            //! @throw exception::limit::Memory on resource failures
            //! @throw exception::system::invalid::Argument for bad input
            //! @throw exception::Casual on other failures
            //!
            std::string encode( const std::string& value);

            //!
            //! @param value The UTF-8 encoded string
            //!
            //! @return String encoded in local default codeset
            //!
            //! @throw exception::limit::Memory on resource failures
            //! @throw exception::system::invalid::Argument for bad input
            //! @throw exception::Casual on other failures
            //!
            std::string decode( const std::string& value);



            //!
            //! @param codeset String-encoding
            //!
            //! @return Whether the provided codeset exist in the system
            //!
            //! @throw exception::limit::Memory on resource failures
            //! @throw exception::Casual on other failures
            //!
            bool exist( const std::string& codeset);


            //!
            //! @param value String encoded in provided codeset
            //! @param codeset String-encoding
            //!
            //! @return UTF-8-encoded string
            //!
            //! @throw exception::limit::Memory on resource failures
            //! @throw exception::system::invalid::Argument for bad input
            //! @throw exception::Casual on other failures
            //!
            std::string encode( const std::string& value, const std::string& codeset);

            //!
            //! @param value The UTF-8 encoded string
            //! @param codeset Encoding for result
            //!
            //! @return String encoded in provided codeset
            //!
            //! @throw exception::limit::Memory on resource failures
            //! @throw exception::system::invalid::Argument for bad input
            //! @throw exception::Casual on other failures
            //!
            std::string decode( const std::string& value, const std::string& codeset);

         } // utf8

         namespace hex
         {
            namespace detail
            {
               std::string encode( const char* first, const char* last);
               void encode( std::ostream& out, const char* first, const char* last);
               void decode( const char* first, const char* last, char* data);
            } // detail


            //!
            //! encode binary sequence [first, last) to hex-representation
            //!
            //! @param first start of binary
            //! @param last end of binary (exclusive)
            //! @return hex-encoded string of [first, last)
            //!
            template< typename Iter>
            std::enable_if_t< traits::is::binary::iterator< Iter>::value, std::string>
            encode( Iter first, Iter last)
            {
               auto cast = []( auto&& i){ return reinterpret_cast< const char*>( &(*i));}; 
               return detail::encode( cast( first), cast( last));
            }

            template< typename Iter>
            std::enable_if_t< traits::is::binary::iterator< Iter>::value, std::ostream&>
            encode( std::ostream& out, Iter first, Iter last)
            {
               auto cast = []( auto&& i){ return reinterpret_cast< const char*>( &(*i));}; 
               detail::encode( out, cast( first), cast( last));
               return out;
            }

            template< typename R>
            auto encode( std::ostream& out, R&& range) -> 
               std::enable_if_t< traits::is::binary::like< R>::value, std::ostream&>
            {
               return encode( out, std::begin( range), std::end( range));
            }

            //!
            //! encode binary @p container to hex-representation
            //!
            //! @param container binary representation
            //! @return hex-encoded string of @p container
            //!
            template< typename C>
            std::string encode( C&& container)
            {
               return encode( std::begin( container), std::end( container));
            }


            //!
            //! decode hex-string to a binary representation
            //!
            //! @param value hex-string
            //! @return binary representation of @p value
            //!
            platform::binary::type decode( const std::string& value);


            template< typename Source, typename Iter>
            std::enable_if_t< 
               traits::is::string::like< Source>::value
               && traits::is::binary::iterator< Iter>::value
            >
            decode( Source&& source, Iter first, Iter last)
            {
               auto size = last - first;
               assert( range::size( source) <= ( size * 2) + 1);

               auto cast_source = []( auto&& i){ return reinterpret_cast< const char*>( &(*i));};
               auto cast_target = []( auto&& i){ return reinterpret_cast< char*>( &(*i));};
               detail::decode( cast_source( std::begin( source)), cast_source( std::end( source)), cast_target( first));
            }


            //!
            //! decode hex-string to a binary representation
            //!
            //! @return binary representation of @p value
            //!
            template< typename Source, typename Target>
            std::enable_if_t< 
               traits::is::string::like< Source>::value
               && traits::is::binary::like< Target>::value
            >
            decode( Source&& source, Target&& target)
            {
               decode( std::forward< Source>( source), std::begin( target), std::end( target));
            }




         } // hex
      } // transcode
   } // common
} // casual



