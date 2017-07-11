//!
//! casual
//!

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

            namespace
            {
               namespace local
               {
                  void check( const pugi::xml_parse_result& result)
                  {
                     if( !result) throw exception::archive::invalid::Document{ result.description()};
                  }

                  namespace empty
                  {
                     const std::string& document()
                     {
                        static std::string empty{ R"(<?xml version="1.0"?><value></value>)"};
                        return empty;
                     }


                  } // empty

               } // local
            }

            Load::Load() = default;
            Load::~Load() = default;

            const pugi::xml_document& Load::operator() () const noexcept
            {
               return m_document;
            }


            const pugi::xml_document& Load::operator() ( std::istream& stream)
            {
               local::check( m_document.load( stream));
               return m_document;
            }

            const pugi::xml_document& Load::operator() ( const std::string& xml)
            {
               if( xml.empty())
               {
                  return operator()( local::empty::document());
               }

               local::check( m_document.load_buffer( xml.data(), xml.size()));
               return m_document;
            }

            const pugi::xml_document& Load::operator() ( const platform::binary::type& xml)
            {
               if( xml.empty())
               {
                  return operator()( local::empty::document());
               }

               local::check( m_document.load_buffer( xml.data(), xml.size()));
               return m_document;
            }

            const pugi::xml_document& Load::operator() ( const char* const xml, const std::size_t size)
            {
               if( ! size || ! xml)
               {
                  return operator()( local::empty::document());
               }

               local::check( m_document.load_buffer( xml, size));
               return m_document;
            }

            const pugi::xml_document& Load::operator() ( const char* const xml)
            {
               if( ! xml || xml[ 0] == '\n')
               {
                  return operator()( local::empty::document());
               }

               //local::check( m_document.load_string( xml));
               local::check( m_document.load( xml));
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
                     auto node =  m_stack.back().child( name);

                     if( node)
                     {
                        m_stack.push_back( m_stack.back().child( name));
                     }
                     else
                     {
                        return false;
                     }
                  }
                  return true;
               }

               void Implementation::end( const char* name)
               {
                  m_stack.pop_back();
               }


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

            pugi::xml_document& Save::operator() () noexcept
            {
               return m_document;
            }

            void Save::operator() ( std::ostream& xml) const
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

            void Save::operator() ( std::string& xml) const
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
               m_document.save( stream, " ");
               xml.assign( stream.str());
            }

            void Save::operator() ( platform::binary::type& xml) const
            {
               std::ostringstream stream;
               m_document.save( stream, " ");
               auto temp = stream.str();
               xml.assign( std::begin( temp), std::end( temp));
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
