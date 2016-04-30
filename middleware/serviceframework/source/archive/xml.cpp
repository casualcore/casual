/*
 * xml.cpp
 *
 *  Created on: Jan 21, 2015
 *      Author: kristone
 */

#include "sf/archive/xml.h"

#include "sf/exception.h"

#include "common/transcode.h"

#include <iterator>
#include <algorithm>

namespace casual
{
   namespace sf
   {

      namespace archive
      {
         namespace xml
         {

            Load::Load() = default;
            Load::~Load() = default;

            namespace
            {
               namespace local
               {
                  void check( const pugi::xml_parse_result& result)
                  {
                     if( !result) throw exception::archive::invalid::Document{ result.description()};
                  }
               } // local
            }


            const pugi::xml_document& Load::serialize( std::istream& stream)
            {
               local::check( m_document.load( stream));
               return source();
            }

            const pugi::xml_document& Load::serialize( const std::string& xml)
            {
               local::check( m_document.load_buffer( xml.data(), xml.size()));
               return source();
            }

            const pugi::xml_document& Load::serialize( const char* const xml, const std::size_t size)
            {
               local::check( m_document.load_buffer( xml, size));
               return source();
            }

            const pugi::xml_document& Load::serialize( const char* const xml)
            {
               //local::check( m_document.load_string( xml));
               local::check( m_document.load( xml));
               return source();
            }

            const pugi::xml_document& Load::source() const noexcept
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
                  // Stack 'em backwards
                  //

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
/*
               namespace
               {
                  namespace local
                  {
                     //
                     // This is a help to check some to avoid terminate (via assert)
                     //
                     template<typename C, typename F>
                     auto read( const rapidjson::Value* const value, C&& checker, F&& fetcher) -> decltype( std::bind( fetcher, value)())
                     {
                        if( std::bind( checker, value)())
                        {
                           return std::bind( fetcher, value)();
                        }
                        else
                        {
                           throw exception::archive::invalid::Node{ "unexpected type"};
                        }
                     }

                  } // local
               } // <unnamed>
*/

               namespace
               {
                  namespace local
                  {
                     template<typename T>
                     T read( const pugi::xml_node& node)
                     {
                        std::istringstream stream( node.text().get());
                        T result;
                        stream >> result;
                        if( ! stream.fail() && stream.eof())   return result;
                        throw exception::archive::invalid::Node{ "unexpected type"};
                     }

                     //
                     // std::istream::eof() is not set when streaming bool
                     //
                     template<>
                     bool read( const pugi::xml_node& node)
                     {
                        const std::string boolean = node.text().get();
                        if( boolean == "true")  return true;
                        if( boolean == "false") return false;
                        throw exception::archive::invalid::Node{ "unexpected type"};
                     }
                  }
               } // <unnamed>


               //
               // Various stox-functions are more cumbersome to use if you
               // wanna make sure the whole content is processed
               //
               void Implementation::read( bool& value) const
               { value = local::read<bool>( m_stack.back()); }
               void Implementation::read( short& value) const
               { value = local::read<short>( m_stack.back()); }
               void Implementation::read( long& value) const
               { value = local::read<long>( m_stack.back()); }
               void Implementation::read( long long& value) const
               { value = local::read<long long>( m_stack.back()); }
               void Implementation::read( float& value) const
               { value = local::read<float>( m_stack.back()); }
               void Implementation::read( double& value) const
               { value = local::read<double>( m_stack.back()); }

               void Implementation::read( char& value) const
               // If empty string this should result in '\0'
               { value = *common::transcode::utf8::decode( m_stack.back().text().get()).c_str(); }
               void Implementation::read( std::string& value) const
               { value = common::transcode::utf8::decode( m_stack.back().text().get()); }
               void Implementation::read( std::vector<char>& value) const
               { value = common::transcode::base64::decode( m_stack.back().text().get()); }

            } // reader


            Save::Save() = default;
            Save::~Save() = default;

            void Save::serialize( std::ostream& xml) const
            {
               m_document.save( xml, " ");
            }

/*
            namespace
            {
               struct string_writer : pugi::xml_writer
               {
                  std::string& string;
                  string_writer( std::string& string) : string( string) {}
                  void save( const void* const data, const std::size_t size) override
                  {
                     string.append( static_cast<const char*>( data), size);
                  }
               };
            }
*/

            void Save::serialize( std::string& xml) const
            {
               //
               // The pugi::xml_writer-interface actually seems to be slower
               //
               //string_writer writer( xml);
               //m_document.save( writer, " ");
               //
               // so we're doin' it in a simpler way instead
               //

               std::ostringstream stream;
               serialize( stream);
               xml.assign( stream.str());
            }

            namespace writer
            {

               Implementation::Implementation( pugi::xml_node node) : m_stack{ std::move( node)} {}

               std::size_t Implementation::container_start( const std::size_t size, const char* const name)
               {
                  start( name);

                  auto element = m_stack.back();

                  //
                  // Stack 'em backwards
                  //

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
                  write( std::string{ value});
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
