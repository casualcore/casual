//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "serviceframework/archive/log.h"
#include "serviceframework/archive/create.h"

#include "common/transcode.h"


#include <iostream>
#include <iomanip>
#include <locale>
#include <algorithm>
//#include <codecvt>

namespace casual
{
   namespace serviceframework
   {

      namespace archive
      {

         namespace log
         {
            namespace local
            {
               namespace
               {            
                  class Implementation
                  {
                  public:

                     static std::vector< std::string> keys() { return { "", "log"};}

                     Implementation();
                     Implementation( std::ostream& out) : m_output( out) {}

                     Implementation( Implementation&&) = default;
                     ~Implementation() 
                     {
                        flush();
                     }


                     platform::size::type container_start( const platform::size::type size, const char* name)
                     {
                        add( name);

                        ++m_indent;

                        m_buffer.back().size = size;
                        m_buffer.back().type = Type::container;
                        flush();

                        return size;
                     }

                     void container_end( const char*)
                     {
                        --m_indent;
                        flush();
                     }

                     void serialtype_start( const char* name)
                     {
                        add( name);
                        ++m_indent;
                        m_buffer.back().type = Type::composite;
                        flush();
                     }
                     
                     void serialtype_end( const char*)
                     {
                        --m_indent;
                        flush();
                     }

                     template<typename T>
                     void write( T value, const char* name)
                     {
                        write( std::to_string( value), name);
                     }

                     void write( std::string&& value, const char* name)
                     {
                        add( name);
                        m_buffer.back().value = std::move( value);
                     }

                     void write( bool value, const char* name)
                     {
                        add( name);
                        m_buffer.back().value = value ? "true" : "false";
                     }

                     void write( char value, const char* name)
                     {
                        add( name);
                        m_buffer.back().value.push_back( value);
                     }

                     void write( const std::string& value, const char* name)
                     {
                        add( name);
                        m_buffer.back().value = value;
                     }

                     void write( const platform::binary::type& value, const char* name)
                     {
                        add( name);
                        m_buffer.back().value = "<binary data> size: " + std::to_string( value.size());
                     }

                  private:

                     void add( const char* name)
                     {
                        if( ! name)
                        {
                           name = "element";
                        }
                        m_buffer.emplace_back( m_indent, name);
                     }

                     void flush()
                     {
                        //
                        // Find the longest name
                        //

                        auto size = common::algorithm::accumulate( m_buffer, 0L, []( auto size, const auto& value){
                           return common::value::max( size, value.name.size());
                        });

                        auto writer = [&]( const auto& value)
                        {
                           m_output << std::right << std::setfill( '|') << std::setw( value.indent);

                           switch( value.type)
                           {
                              case Type::composite:
                              {
                                 m_output << '-' << value.name;
                                 break;
                              }
                              case Type::container:
                              {
                                 m_output << '-' << value.name;
                                 if( value.size) { m_output << " (size: " << value.size << ')';}
                                 break;
                              }
                              default:
                              {
                                 m_output << '-' << value.name<< std::setfill( '.') << std::setw( ( size + 3) - value.name.size()) << '[' << value.value << ']';
                                 break;
                              }
                           }
                           m_output << '\n';
                        };

                        std::for_each( std::begin( m_buffer), std::end( m_buffer), writer);

                        m_buffer.clear();
                     }


                     enum class Type
                     {
                        value,
                        container,
                        composite
                     };

                     struct buffer_type
                     {
                        buffer_type( platform::size::type indent, const char* name) : indent( indent), name( name) {}
                        buffer_type( buffer_type&&) = default;

                        platform::size::type indent;
                        std::string name;
                        std::string value;
                        platform::size::type size = 0;
                        Type type = Type::value;
                     };

                     std::ostream& m_output;
                     std::vector< buffer_type> m_buffer;
                     platform::size::type m_indent = 1;
                  };
               } // <unnamed>
            } // local


            Writer writer( std::ostream& out)
            {
               return Writer::emplace< local::Implementation>( out);
            }

         } // log

         namespace create
         {
            namespace writer
            {
               template struct Registration< log::local::Implementation>;
            } // writer
         } // create

      } // archive
   } // serviceframework
} // casual
