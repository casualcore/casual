/*
 * xml.h
 *
 *  Created on: Jan 15, 2015
 *      Author: kristone
 */

#ifndef CASUAL_SF_ARCHIVE_XML_H_
#define CASUAL_SF_ARCHIVE_XML_H_

#include <iosfwd>
#include <sstream>
#include <vector>
#include <iterator>
#include <algorithm>
#include <utility>
#include <type_traits>
#include <tuple>

#include "sf/archive/basic.h"
#include "sf/exception.h"

#include "common/transcode.h"

#include <pugixml.hpp>


namespace casual
{
   namespace sf
   {
      namespace archive
      {
         namespace xml
         {
            //
            // This implementation uses pugixml 1.2
            //
            // There are some flaws in this implementation and we're waiting
            // for libpugixml-dev with 1.4 so they can be fixed
            //
            // The 1.4/1.5 do offer a slight different API, so we need to adapt
            // to it to fix the flaws
            //

            namespace reader
            {

               class Implementation
               {
               public:


                  //!
                  //! @param node Normally an pugi::xml_document
                  //!
                  explicit Implementation( pugi::xml_node node ) : m_stack{ std::move( node)} {}

                  inline std::tuple< std::size_t, bool> container_start( const std::size_t size, const char* const name)
                  {
                     if( name)
                     {
                        m_stack.push_back( m_stack.back().child( name));

                        if( ! m_stack.back())
                        {
                           return std::make_tuple( 0, false);
                        }

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

                  inline void container_end( const char* const name)
                  {
                     m_stack.pop_back();
                  }

                  inline bool serialtype_start( const char* const name)
                  {
                     if( name)
                     {
                        m_stack.push_back( m_stack.back().child( name));
                     }

                     return m_stack.back();

                  }

                  inline void serialtype_end( const char* const name)
                  {
                     m_stack.pop_back();
                  }


                  template< typename T>
                  bool read( T& value, const char* const name)
                  {
                     if( name)
                     {
                        if( const auto element = m_stack.back().child( name))
                        {
                           m_stack.push_back( element);
                        }
                        else
                        {
                           return false;
                        }
                     }
                     read( value);

                     m_stack.pop_back();

                     return true;

                  }

               private:

                  //
                  // TODO: Add some error handling
                  //

                  template<typename T>
                  void read( T& value) const
                  {
                     std::istringstream stream( m_stack.back().text().get());
                     stream >> value;
                  }

                  void read( bool& value) const
                  {
                     std::istringstream stream( m_stack.back().text().get());
                     stream >> std::boolalpha >> value;
                  }

                  void read( char& value) const
                  {
                     const auto string = common::transcode::utf8::decode( m_stack.back().text().get());
                     value = string.empty() ? '\0' : string.front();
                  }

                  void read( std::string& value) const
                  {
                     value = common::transcode::utf8::decode( m_stack.back().text().get());
                  }

                  void read( std::vector<char>& value) const
                  {
                     value = common::transcode::base64::decode( m_stack.back().text().get());
                  }


               private:

                  // 'vector' instead of 'stack' to use some algorithms
                  std::vector<pugi::xml_node> m_stack;

               }; // Implementation

            } // reader

            namespace writer
            {

               class Implementation
               {

               public:

                  //!
                  //! @param node Normally an xml_document
                  //!
                  Implementation( pugi::xml_node node)
                  {
                     m_stack.push( std::move( node));
                  }

                  inline std::size_t container_start( const std::size_t size, const char* const name)
                  {
                     if( name)
                     {
                        m_stack.push( m_stack.top().append_child( name));
                     }

                     auto element = m_stack.top();

                     for( std::size_t idx = 0; idx < size; ++idx)
                     {
                        m_stack.push( element.prepend_child( "element"));
                     }

                     return size;
                  }

                  inline void container_end( const char* const name)
                  {
                     m_stack.pop();
                  }

                  inline void serialtype_start( const char* const name)
                  {
                     if( name)
                     {
                        m_stack.push( m_stack.top().append_child( name));
                     }
                  }

                  inline void serialtype_end( const char* const name)
                  {
                     m_stack.pop();
                  }

                  template< typename T>
                  void write( const T& value, const char* const name)
                  {
                     if( name)
                     {
                        m_stack.push( m_stack.top().append_child( name));
                     }

                     write( value);

                     m_stack.pop();
                  }


               private:

                  template<typename T>
                  //typename std::enabe_if<std::is_floating_point<T>::value, void>::type
                  void write( const T& value)
                  {
                     std::ostringstream stream;
                     stream << std::fixed << value;
                     m_stack.top().text().set( stream.str().c_str());
                  }

                  void write( const bool& value)
                  {
                     std::ostringstream stream;
                     stream << std::boolalpha << value;
                     m_stack.top().text().set( stream.str().c_str());
                  }

                  void write( const char& value)
                  {
                     m_stack.top().text().set( common::transcode::utf8::encode( std::string( 1, value)).c_str());
                  }

                  void write( const std::string& value)
                  {
                     m_stack.top().text().set( common::transcode::utf8::encode( value).c_str());
                  }

                  void write( const std::vector<char>& value)
                  {
                     m_stack.top().text().set( common::transcode::base64::encode( value).c_str());
                  }

               private:

                  std::stack<pugi::xml_node> m_stack;

               }; // Implementation

            } // writer

            namespace relaxed
            {
               typedef basic_reader< reader::Implementation, policy::Relaxed> Reader;
            }

            typedef basic_reader< reader::Implementation, policy::Strict > Reader;

            typedef basic_writer< writer::Implementation> Writer;


            //
            // Load
            //
            // TODO: Better
            //

            //typedef pugi::xml_document reader_type;
            typedef pugi::xml_document source_type;

            static void load( pugi::xml_document& document, std::istream& xml)
            {
               const auto result = document.load( xml);
               if( !result) throw exception::archive::invalid::Document{ result.description()};
            }

            static void load( pugi::xml_document& document, const std::vector<char>& xml)
            {
               const auto result = document.load_buffer( xml.data(), xml.size());
               if( !result) throw exception::archive::invalid::Document{ result.description()};
            }

            static void load( pugi::xml_document& document, const std::string& xml)
            {
               const auto result = document.load_buffer( xml.data(), xml.size());
               if( !result) throw exception::archive::invalid::Document{ result.description()};
            }

            static void load( pugi::xml_document& document, const char* const xml)
            {
               const auto result = document.load( xml);
               if( !result) throw exception::archive::invalid::Document{ result.description()};
            }


            //
            // Save
            //
            // TODO: Better
            //

            //typedef pugi::xml_document writer_type;
            typedef pugi::xml_document target_type;

            static void save( const pugi::xml_document& document, std::ostream& xml)
            {
               document.save( xml, " ");
            }

            static void save( const pugi::xml_document& document, std::string& xml)
            {
               //
               // TODO: Implement the xml_writer-interface
               //

               std::ostringstream stream;
               document.save( stream, " ");
               xml = stream.str();
            }


         } // xml
      } // archive
   } // sf
} // casual



#endif /* CASUAL_SF_ARCHIVE_XML_H_ */
