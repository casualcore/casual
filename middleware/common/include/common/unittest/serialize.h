//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/serialize/create.h"
#include "common/serialize/value.h"

#include <string_view>

namespace casual
{
   namespace common::unittest::serialize
   {
      namespace create
      {
         namespace detail
         {
            template< typename T, typename I>
            auto value( std::string_view format, I&& information, traits::priority::tag< 1>)
               -> decltype( void( common::serialize::create::reader::consumed::from( format, std::forward< I>( information))), T{})
            {
               auto reader = common::serialize::create::reader::consumed::from( format, std::forward< I>( information));
               T value;
               reader >> value;
               reader.validate();
               return value;
            }

            template< typename T>
            auto value( std::string_view format, std::string_view information, traits::priority::tag< 0>)
            {
               // crete a binary type from the string and call the one above
               auto binary = platform::binary::type{ std::begin( information), std::end( information)};
               return value< T>( format, binary, traits::priority::tag< 1>{});
            }
            
         } // detail
         template< typename T, typename I>
         auto value( std::string_view format, I&& information)
            -> decltype( detail::value< T>( format, std::forward< I>( information), traits::priority::tag< 1>{}))
         {
            common::Trace trace{ "common::unittest::serialize::create::value"};
            return detail::value< T>( format, std::forward< I>( information), traits::priority::tag< 1>{});
         }
      } // create


      namespace detail::hash
      {
         struct Writer
         {
            using value_type = decltype( std::hash< long>{}( 0l));
          
            inline constexpr static auto archive_type() { return common::serialize::archive::Type::static_order_type;}

            inline platform::size::type container_start( const platform::size::type size, const char* name)
            {
               add( size);
               return size;
            }

            inline void container_end( const char*) { /* no-op */};
            inline void composite_start( const char* name) { /* no-op */};
            inline void composite_end( const char*) { /* no-op */};

            template<typename T>
            auto write( T&& value, traits::priority::tag< 1>)
               -> decltype( void( std::hash< traits::remove_cvref_t< T>>{}( value)))
            {
               add( std::forward< T>( value));
            }

            template<typename T>
            auto write( T&& value, traits::priority::tag< 0>)
               -> decltype( void( string::view::make( value)))
            {
               write( string::view::make( value), traits::priority::tag< 1>{});
            }

            template<typename T>
            auto write( T&& value, const char*)
               -> decltype( write( std::forward< T>( value), traits::priority::tag< 1>{}))
            {
               write( std::forward< T>( value), traits::priority::tag< 1>{});
            }

            auto operator () () const { return m_hash;}

            template< typename T>
            Writer& operator << ( T&& value)
            {
               common::serialize::value::write( *this, std::forward< T>( value), nullptr);
               return *this;
            }

         private:

            template< typename T>
            inline void add( T&& value)
            {
               // stolen from: https://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x
               // assumes that the 'boost guys' knows this stuff and have tested the entropy
               std::hash< traits::remove_cvref_t< T>> hasher;
               m_hash ^= hasher( value) + 0x9e3779b9 + (m_hash<<6) + (m_hash>>2);
            }

            value_type m_hash{};
         };
         
      } // detail::hash

      template< typename T>
      auto hash( T&& value)
      {
         detail::hash::Writer hasher;
         hasher << value;
         return hasher();
      }

   } // common::unittest::serialize
} // casual