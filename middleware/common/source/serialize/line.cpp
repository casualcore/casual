//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/serialize/line.h"

#include "common/platform.h"
#include "common/transcode.h"
#include "common/log.h"

#include <ostream>
#include <iomanip>

namespace casual
{
   namespace common
   {
      namespace serialize
      {
         namespace line
         {

            std::ostream& Writer::maybe_name( std::ostream& stream, const char* name)
            {
               if( name)
                  stream << name << ": ";

               return stream;
            }

            platform::size::type Writer::container_start( const platform::size::type size, const char* name)
            {
               begin_scope();
               maybe_name( m_stream, name) << ( size == 0 ? "[" : "[ ");
               return size;
            }

            void Writer::container_end( const char*)
            {
               m_stream << ']';
               m_prefix = detail::scope;
            }

            void Writer::composite_start( const char* name)
            {
               begin_scope();
               maybe_name( m_stream, name) << "{ ";
            }
            
            void Writer::composite_end( const char*)
            {
               m_stream << '}';
               m_prefix = detail::scope;
            }

            void Writer::write( bool value, const char* name)
            {
               in_scope();
               maybe_name( m_stream, name) << ( value ? "true" : "false"); 
            }

            void Writer::write( view::immutable::Binary value, const char* name)
            {
               in_scope();
               maybe_name( m_stream, name);
               
               if( value.size() > 32) 
                  m_stream << "\"binary size: " << value.size() << '"';
               else
               {
                  transcode::hex::encode( m_stream, value);
               }
            }

            void Writer::write( const platform::binary::type& value, const char* name)
            {
               write( view::binary::make( value), name);
            }

            void Writer::write( const std::string& value, const char* name)
            {
               in_scope();
               maybe_name( m_stream, name) << std::quoted( value);
            }

            void Writer::begin_scope()
            {
               m_stream << std::exchange( m_prefix, detail::first);
            }

            void Writer::in_scope()
            {
               m_stream << std::exchange( m_prefix, detail::scope);
            }

         } // log
      } // serialize
   } // common
} // casual