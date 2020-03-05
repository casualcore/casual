//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/serialize/value.h"
#include "common/serialize/archive.h"
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

               static std::vector< std::string> keys();

               platform::size::type container_start( const platform::size::type size, const char* name);
               void container_end( const char*);

               void composite_start( const char* name);
               void composite_end( const char*);

               template<typename T>
               auto write( T&& value, const char* name)
                  -> decltype( void( std::declval< std::ostream&>() << std::forward< T>( value)))
               {
                  in_scope();
                  maybe_name( name) << std::forward< T>( value);
               }

               void write( bool value, const char* name);
               void write( view::immutable::Binary value, const char* name);
               void write( const platform::binary::type& value, const char* name);
               void write( const std::string& value, const char* name);


               template< typename T>
               auto operator << ( T&& value)
                  -> decltype( (void)serialize::value::write( *this, std::forward< T>( value), nullptr), *this)
               {
                  serialize::value::write( *this, std::forward< T>( value), nullptr);
                  return *this;
               }

               template< typename T>
               Writer& operator & ( T&& value)
               {
                  return *this << std::forward< T>( value);
               }

               void consume( std::ostream& destination);
               std::string consume();

            private:

               std::ostream& maybe_name( const char* name);

               void begin_scope();
               void in_scope();

               std::ostringstream m_stream;
               const char* m_prefix = detail::first;
            };

            //! type erased line writer 
            serialize::Writer writer();

         } // line

      } // serialize
   } // common
} // casual




