

#include "sf/archive/log.h"

#include "common/transcode.h"


#include <iostream>
#include <iomanip>
#include <locale>
#include <algorithm>
//#include <codecvt>

namespace casual
{
   namespace sf
   {

      namespace archive
      {

         namespace log
         {

            Implementation::Implementation( std::ostream& out) : m_output( out)
            {

            }

            Implementation::~Implementation()
            {
               flush();
            }


            std::size_t Implementation::container_start( const std::size_t size, const char* name)
            {
               add( name);

               ++m_indent;

               m_buffer.back().type = Type::container;
               flush();

               return size;
            }

            void Implementation::container_end( const char*)
            {
               --m_indent;
               flush();
            }

            void Implementation::serialtype_start( const char* name)
            {
               add( name);
               ++m_indent;
               m_buffer.back().type = Type::composite;
               flush();
            }

            void Implementation::serialtype_end( const char*)
            {
               --m_indent;
               flush();
            }




            void Implementation::write( std::string&& value, const char* name)
            {
               add( name);
               m_buffer.back().value = std::move( value);
            }

            void Implementation::write( const bool& value, const char* name)
            {
               add( name);

               m_buffer.back().value = value ? "true" : "false";
            }

            void Implementation::write( const std::string& value, const char* name)
            {
               add( name);
               m_buffer.back().value = value;
            }

            void Implementation::write( const std::wstring& value, const char* name)
            {
               add( name);
               m_buffer.back().value = "wide string (" + std::to_string( value.size()) + ")";
            }

            void Implementation::write( const platform::binary_type& value, const char* name)
            {
               add( name);
               m_buffer.back().value = common::transcode::base64::encode( value);
            }

            void Implementation::add( const char* name)
            {
               if( ! name)
               {
                  name = "element";
               }
               m_buffer.emplace_back( m_indent, name);
            }

            void Implementation::flush()
            {
               //
               // Find the longest name
               //
               std::size_t size = 0;

               auto maxSize = [&]( const Implementation::buffer_type& value)
               {
                  if( value.name.size() > size) size = value.name.size();
               };

               std::for_each( std::begin( m_buffer), std::end( m_buffer), maxSize);


               auto writer = [&]( const Implementation::buffer_type& value)
               {
                  m_output << std::right << std::setfill( '|') << std::setw( value.indent);

                  switch( value.type)
                  {
                     case Type::composite:
                     case Type::container:
                     {
                        m_output << '-' << value.name;
                        break;
                     }
                     default:
                     {
                        m_output << '-' << value.name<< std::setfill( '.') << std::setw( ( size + 3) - value.name.size()) << '[' << value.value << ']';
                        break;
                     }
                  }
                  m_output << std::endl;
               };

               std::for_each( std::begin( m_buffer), std::end( m_buffer), writer);

               m_buffer.clear();
            }

         } // logger

      } // archive

   } // sf

} // casual
