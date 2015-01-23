/*
 * xml.cpp
 *
 *  Created on: Jan 21, 2015
 *      Author: kristone
 */

#include "sf/archive/xml.h"

#include "sf/exception.h"

#include "common/transcode.h"

namespace casual
{
   namespace sf
   {

      namespace archive
      {
         namespace xml
         {
            namespace
            {
               void check( const pugi::xml_parse_result& result)
               {
                  if( !result) throw exception::archive::invalid::Document{ result.description()};
               }
            }

            Load::Load() = default;
            Load::~Load() = default;

            void Load::serialize( std::istream& stream)
            {
               check( m_document.load( stream));
            }

            void Load::serialize( const std::string& xml)
            {
               check( m_document.load_buffer( xml.data(), xml.size()));
            }

            void Load::serialize( const char* const xml)
            {
               //check( m_document.load_string( xml));
               check( m_document.load( xml));
            }



            const pugi::xml_document& Load::source() const
            {
               return m_document;
            }



            namespace reader
            {

               Implementation::Implementation( pugi::xml_node node ) : m_stack{ std::move( node)} {}

               std::tuple< std::size_t, bool> Implementation::container_start( const std::size_t size, const char* const name)
               {
                  if( ! start( name))
                  {
                     return std::make_tuple( 0, false);
                  }

                  //
                  // TODO: We need to filter elements not named 'element',
                  // but with 1.4 that is really simple, so we just wait
                  //


                  // 1.2
                  const auto content = m_stack.back().children();
                  // 1.4 (with bidirectional xml_named_node_iterator)
                  //const auto content = m_stack.back().children( "element");
                  std::reverse_copy( std::begin( content), std::end( content), std::back_inserter( m_stack));

                  return std::make_tuple( std::distance( std::begin( content), std::end( content)), true);

               }

               void Implementation::container_end( const char* const name)
               {
                  end( name);
               }

               bool Implementation::serialtype_start( const char* const name)
               {
                  return start( name);
               }

               void Implementation::serialtype_end( const char* const name)
               {
                  end( name);
               }

               bool Implementation::start( const char* const name)
               {
                  if( name)
                  {
                     m_stack.push_back( m_stack.back().child( name));
                  }
                  else
                  {
                     return true;
                  }

                  return m_stack.back();
               }

               void Implementation::end( const char* name)
               {
                  m_stack.pop_back();
               }


               void Implementation::read( bool& value) const
               {
                  std::istringstream stream( m_stack.back().text().get());
                  stream >> std::boolalpha >> value;
               }

               void Implementation::read( char& value) const
               {
                  const auto string = common::transcode::utf8::decode( m_stack.back().text().get());
                  value = string.empty() ? '\0' : string.front();
               }

               void Implementation::read( std::string& value) const
               {
                  value = common::transcode::utf8::decode( m_stack.back().text().get());
               }

               void Implementation::read( std::vector<char>& value) const
               {
                  value = common::transcode::base64::decode( m_stack.back().text().get());
               }


            } // reader


            Save::Save() = default;
            Save::~Save() = default;

            void Save::serialize( std::ostream& stream) const
            {
               m_document.save( stream);
            }

            void Save::serialize( std::string& xml) const
            {
               // TODO: implement the xml_writer-interface
               std::ostringstream stream;
               m_document.save( stream);
               xml.assign( stream.str());
            }

            const pugi::xml_document& Save::target() const
            {
               return m_document;
            }


            namespace writer
            {

               Implementation::Implementation( pugi::xml_node node) : m_stack{ std::move( node)} {}

               std::size_t Implementation::container_start( const std::size_t size, const char* const name)
               {
                  start( name);

                  auto element = m_stack.back();

                  for( std::size_t idx = 0; idx < size; ++idx)
                  {
                     m_stack.push_back( element.prepend_child( "element"));
                  }

                  return size;
               }

               void Implementation::container_end( const char* const name)
               {
                  end( name);
               }

               void Implementation::serialtype_start( const char* const name)
               {
                  start( name);
               }

               void Implementation::serialtype_end( const char* const name)
               {
                  end( name);
               }

               void Implementation::start( const char* const name)
               {
                  if( name)
                  {
                     m_stack.push_back( m_stack.back().append_child( name));
                  }
               }

               void Implementation::end( const char* const name)
               {
                  m_stack.pop_back();
               }

               void Implementation::write( const bool& value)
               {
                  std::ostringstream stream;
                  stream << std::boolalpha << value;
                  m_stack.back().text().set( stream.str().c_str());
               }

               void Implementation::write( const char& value)
               {
                  write( std::string( 1, value));
               }

               void Implementation::write( const std::string& value)
               {
                  m_stack.back().text().set( common::transcode::utf8::encode( value).c_str());
               }

               void Implementation::write( const std::vector<char>& value)
               {
                  m_stack.back().text().set( common::transcode::base64::encode( value).c_str());
               }


            } // writer


         } // xml

      } // archive
   } // sf

} // casual
