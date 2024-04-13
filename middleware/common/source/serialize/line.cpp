//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/serialize/line.h"

#include "common/serialize/create.h"

#include "casual/platform.h"
#include "common/transcode.h"
#include "common/log.h"

#include <ostream>
#include <iomanip>

namespace casual
{
   namespace common::serialize
   {
      namespace line
      {
         Writer::Writer()
         {
            m_stream.precision( std::numeric_limits< double >::max_digits10);
            m_stream.setf( std::ios_base::fixed, std::ios_base::floatfield);
         }

         std::ostream& Writer::maybe_name( const char* name)
         {
            if( name)
               m_stream << name << ": ";  

            return m_stream;
         }

         platform::size::type Writer::container_start( const platform::size::type size, const char* name)
         {
            begin_scope();
            maybe_name( name) << ( size == 0 ? "[" : "[ ");
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
            maybe_name( name) << "{ ";
         }
         
         void Writer::composite_end( const char*)
         {
            m_stream << '}';
            m_prefix = detail::scope;
         }

         void Writer::save( bool value, const char* name)
         {
            maybe_name( name) << ( value ? "true" : "false"); 
         }

         void Writer::save( view::immutable::Binary value, const char* name)
         {
            maybe_name( name);
            
            if( value.size() > 32) 
               m_stream << "\"binary size: " << value.size() << '"';
            else
            {
               transcode::hex::encode( m_stream, value);
            }
         }

         void Writer::save( const platform::binary::type& value, const char* name)
         {
            save( view::binary::make( value), name);
         }

         void Writer::save( const std::string& value, const char* name)
         {
            maybe_name( name) << value;
         }

         void Writer::save( const std::u8string& value, const char* name)
         {
            save( transcode::utf8::decode( value), name);
         }

         void Writer::begin_scope()
         {
            m_stream << std::exchange( m_prefix, detail::first);
         }

         void Writer::in_scope()
         {
            m_stream << std::exchange( m_prefix, detail::scope);
         }

         void Writer::consume( std::ostream& destination)
         {
            destination << consume();
            // TODO conformance: is ostringstream::rdbuf "special"?
            // destination << m_stream.rdbuf(); does not work fully... 
         }

         std::string Writer::consume()
         {
            return std::exchange( m_stream, std::ostringstream{}).str();
         }
         
         serialize::Writer writer()
         {
            return serialize::Writer::emplace< line::Writer>();
         }
      } // line

      namespace create
      {
         namespace writer
         {
            template struct Registration< line::Writer>;
         } // writer
      } // create

   } // common::serialize
} // casual