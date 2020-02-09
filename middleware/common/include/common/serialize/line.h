//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/serialize/value.h"
#include "common/serialize/archive/type.h"
#include "common/cast.h"

#include <ostream>

namespace casual
{
   namespace common
   {
      namespace serialize
      {
         namespace line
         {
            namespace detail
            {
               constexpr auto first = "";
               constexpr auto scope = ", ";
            } // detail

            struct Writer
            {
               inline constexpr static auto archive_type() { return archive::Type::static_need_named;}

               Writer( std::ostream& stream) : m_stream( stream) {}

               platform::size::type container_start( const platform::size::type size, const char* name);
               void container_end( const char*);

               void composite_start( const char* name);
               void composite_end( const char*);

               template<typename T>
               auto write( T&& value, const char* name)
                  -> decltype( void( std::declval< std::ostream&>() << std::forward< T>( value)))
               {
                  in_scope();
                  maybe_name( m_stream, name) << std::forward< T>( value);
               }

               void write( bool value, const char* name);
               void write( view::immutable::Binary value, const char* name);
               void write( const platform::binary::type& value, const char* name);
               void write( const std::string& value, const char* name);


               template< typename T>
               friend auto operator << ( Writer& out, T&& value)
                  -> decltype( (void)serialize::value::write( out, std::forward< T>( value), nullptr), out)
               {
                  serialize::value::write( out, std::forward< T>( value), nullptr);
                  return out;
               }

               template< typename T>
               Writer& operator & ( T&& value)
               {
                  return *this << std::forward< T>( value);
               }

            private:

               static std::ostream& maybe_name( std::ostream& stream, const char* name);

               void begin_scope();
               void in_scope();

               std::ostream& m_stream;
               const char* m_prefix = detail::first;
            };

         } // line

      } // serialize
   } // common
} // casual




