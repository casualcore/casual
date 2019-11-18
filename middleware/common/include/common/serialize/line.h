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
               //constexpr auto init = "";
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
               void write( const T& value, const char* name)
               {
                  in_scope();
                  maybe_name( m_stream, name) << value;
               }

               void write( bool value, const char* name);
               void write( view::immutable::Binary value, const char* name);
               void write( const platform::binary::type& value, const char* name);
               void write( const std::string& value, const char* name);


               template< typename T>
               Writer& operator << ( T&& value)
               {
                  serialize::value::write( *this, std::forward< T>( value), nullptr);
                  return *this;
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

         //! Specialization for enum
         template< typename T>
         struct Value< T, line::Writer, std::enable_if_t< std::is_enum< T>::value>>
         {

            template< typename V> 
            static void write( line::Writer& archive, V value, const char* name)
            {
               // dispatch to prioritized defined stream operator, and fallback
               // to cast to underlying
               priority_write( archive, value, name, traits::priority::tag< 2>{});
            }

         private:
            template< typename V> 
            static auto priority_write( 
               line::Writer& archive, 
               V value, 
               const char* name, 
               traits::priority::tag< 0>)
            {
               archive.write( cast::underlying( value), name);
            }

            template< typename V> 
            static auto priority_write( 
               line::Writer& archive, 
               V value, 
               const char* name, 
               traits::priority::tag< 1>) -> decltype( std::declval< std::ostream&>() << value, void())
            {
               archive.write( value, name);
            }

            // TODO feature: we may need to dispatch on error_code also
         };

      } // serialize
   } // common
} // casual




