//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "sf/archive/line.h"

namespace casual
{
   namespace sf
   {

      namespace archive
      {
         namespace local
         {
            namespace
            {

               constexpr auto quote = '\"';
               constexpr auto first = "";
               constexpr auto scope = ", ";

               std::ostream& maybe_name( std::ostream& stream, const char* name)
               {
                  if( name)
                  {
                     stream <<  name  << ": ";
                  }
                  return stream;
               }

               struct Implementation
               {
                  Implementation( std::ostream& stream) : m_stream( stream) {}

                  platform::size::type container_start( const platform::size::type size, const char* name)
                  {
                     begin_scope();
                     maybe_name( m_stream, name) << "[ ";
                     return size;
                  }

                  void container_end( const char*)
                  {
                     m_stream << ']';
                  }

                  void serialtype_start( const char* name)
                  {
                     begin_scope();
                     maybe_name( m_stream, name) << "{ ";
                  }
                  
                  void serialtype_end( const char*)
                  {
                     m_stream << '}';
                  }

                  template<typename T>
                  void write( T&& value, const char* name)
                  {
                     in_scope();
                     maybe_name( m_stream, name) << value;
                  }

                  void write( bool value, const char* name)
                  {
                     in_scope();
                     maybe_name( m_stream, name) << ( value ? "true" : "false"); 
                  }

                  void write( const platform::binary::type& value, const char* name)
                  {
                     in_scope();
                     maybe_name( m_stream, name) << quote << "binary size: " << value.size() << quote;
                  }

                  void write( const std::string& value, const char* name)
                  {
                     in_scope();
                     maybe_name( m_stream, name) << quote << value << quote;
                  }

               private:

                  void begin_scope()
                  {
                     m_stream << std::exchange( m_prefix, local::first);
                  }

                  void in_scope()
                  {
                     m_stream << std::exchange( m_prefix, local::scope);
                  }


                  std::ostream& m_stream;
                  const char* m_prefix = local::first;
               };
            } // <unnamed>
         } // local

         namespace line
         {
            Writer writer( std::ostream& out)
            {
               return Writer::emplace< local::Implementation>( out);
            }

         } // log
      } // archive
   } // sf
} // casual