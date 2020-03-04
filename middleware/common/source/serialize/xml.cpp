//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/serialize/xml.h"
#include "common/serialize/create.h"

#include "common/exception/casual.h"
#include "common/transcode.h"
#include "common/buffer/type.h"

#include <pugixml.hpp>

#include <iterator>
#include <algorithm>


namespace casual
{
   namespace common
   {
      namespace serialize
      {
         namespace xml
         {
            namespace local
            {
               namespace
               {
                  std::vector< std::string> keys() { return { "xml", common::buffer::type::xml()};};

                  // This implementation uses pugixml 1.2
                  //
                  // There are some flaws in this implementation and we're waiting
                  // for libpugixml-dev with 1.4 so they can be fixed
                  //
                  // The 1.4/1.5 do offer a slight different API, so we need to adapt
                  // to it to fix the flaws

                  namespace reader
                  {

                     namespace empty
                     {
                        constexpr const auto document =  R"(<?xml version="1.0"?><value></value>)";
                     } // empty

                     struct Load
                     {
                        const pugi::xml_document& operator () ( pugi::xml_document& document, std::istream& stream)
                        {
                           check( document.load( stream));
                           return document;
                        }

                        const pugi::xml_document& operator () ( pugi::xml_document& document, const std::string& xml)
                        {
                           if( xml.empty())
                           {
                              return operator ()( document, empty::document);
                           }
                           check( document.load_buffer( xml.data(), xml.size()));
                           return document;
                        }

                        const pugi::xml_document& operator () ( pugi::xml_document& document, const platform::binary::type& xml)
                        {
                           if( xml.empty())
                           {
                              return operator ()( document, empty::document);
                           }
                           check( document.load_buffer( xml.data(), xml.size()));
                           return document;
                        }

                        const pugi::xml_document& operator () ( pugi::xml_document& document, const char* const xml, const platform::size::type size)
                        {
                           if( ! size || ! xml)
                           {
                              return operator ()( document, empty::document);
                           }
                           check( document.load_buffer( xml, size));
                           return document;
                        }

                        const pugi::xml_document& operator () ( pugi::xml_document& document, const char* const xml)
                        {
                           if( ! xml || xml[ 0] == '\n')
                           {
                              return operator ()( document, empty::document);
                              ;
                           }

                           //local::check( m_document.load_string( xml));
                           check( document.load( xml));
                           return document;
                        }

                     private:
                        void check( const pugi::xml_parse_result& result)
                        {
                           if( !result) throw exception::casual::invalid::Document{ result.description()};
                        }
                     };

                     namespace canonical
                     {
                        using Node = pugi::xml_node;

                        namespace filter
                        {
                           auto children( const Node& node)
                           {
                              auto filter_child = []( const auto& child){
                                 return child.type() == pugi::xml_node_type::node_element;
                              };

                              return common::algorithm::filter( node.children(), filter_child);
                           }
                        } // filter

                        struct Parser 
                        {
                           auto operator() ( const Node& document)
                           {
                              // take care of the document node
                              for( auto& child : filter::children( document))
                              {
                                 element( child);
                              }
                              
                              return std::exchange( m_canonical, {});
                           }

                        private:

                           void element( const Node& node)
                           {
                              auto children = filter::children( node);

                              if( children)
                              {
                                 m_canonical.composite_start( node.name());

                                 for( auto& child : children)
                                 {
                                    element( child);
                                 }

                                 m_canonical.composite_end();
                              }
                              else 
                              {
                                 // we only add if there is something in it
                                 if( ! node.text().empty())
                                    m_canonical.attribute( node.name());
                              }
                           }
                           policy::canonical::Representation m_canonical;
                        };

                        auto parse( const pugi::xml_node& document)
                        {
                           return Parser{}( document);
                        }

                     } // canonical

                     class Implementation
                     {
                     public:

                        inline constexpr static auto archive_type() { return archive::Type::static_need_named;}

                        static decltype( auto) keys() { return local::keys();}

                        //! @param node Normally a pugi::xml_document
                        //!
                        //! @note Any possible document has to outlive the reader
                        template< typename... Ts>
                        explicit Implementation( Ts&&... ts) : m_stack{ Load{}( m_document, std::forward< Ts>( ts)...)} {}

                        std::tuple< platform::size::type, bool> container_start( const platform::size::type size, const char* const name)
                        {
                           if( ! start( name))
                           {
                              return std::make_tuple( 0, false);
                           }

                           // Stack 'em backwards

                           // TODO: We need to filter elements not named 'element',
                           // but with 1.4 that is really simple, so we just wait

                           // 1.2
                           const auto content = m_stack.back().children();
                           // 1.4 (with bidirectional xml_named_node_iterator)
                           //const auto content = m_stack.back().children( "element");
                           std::reverse_copy( std::begin( content), std::end( content), std::back_inserter( m_stack));

                           return std::make_tuple( std::distance( std::begin( content), std::end( content)), true);

                        }

                        void container_end( const char* const name)
                        {
                           end( name);
                        }

                        bool composite_start( const char* const name)
                        {
                           return start( name);
                        }

                        void composite_end(  const char* const name)
                        {
                           end( name);
                        }


                        template< typename T>
                        bool read( T& value, const char* const name)
                        {
                           if( ! start( name))
                              return false;

                           read( value);
                           end( name);

                           return true;
                        }

                        policy::canonical::Representation canonical()
                        {
                           return canonical::parse( m_document);
                        }

                     private:

                        bool start( const char* const name)
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

                        void end( const char* name)
                        {
                           m_stack.pop_back();
                        }

                        // Various stox-functions are more cumbersome to use if you
                        // wanna make sure the whole content is processed
                        void read( bool& value) const
                        {
                           const std::string boolean = m_stack.back().text().get();
                           if( boolean == "true") 
                              value = true;
                           else if( boolean == "false") 
                              value = false;
                           else throw exception::casual::invalid::Node{ "unexpected type"};
                        }

                        template<typename T>
                        T extract( const pugi::xml_node& node) const
                        {
                           std::istringstream stream( node.text().get());
                           T result;
                           stream >> result;
                           if( ! stream.fail() && stream.eof())   return result;
                           throw exception::casual::invalid::Node{ "unexpected type"};
                        }

                        void read( short& value) const
                        { value = extract< short>( m_stack.back()); }
                        void read( long& value) const
                        { value = extract< long>( m_stack.back()); }
                        void read( long long& value) const
                        { value = extract< long long>( m_stack.back()); }
                        void read( float& value) const
                        { value = extract< float>( m_stack.back()); }
                        void read( double& value) const
                        { value = extract< double>( m_stack.back()); }

                        void read( char& value) const
                        // If empty string this should result in '\0'
                        { value = *common::transcode::utf8::decode( m_stack.back().text().get()).c_str(); }
                        void read( std::string& value) const
                        { value = common::transcode::utf8::decode( m_stack.back().text().get()); }
                        
                        void read( std::vector<char>& value) const
                        { value = common::transcode::base64::decode( m_stack.back().text().get()); }

                        void read( view::Binary value) const
                        { 
                           auto binary = common::transcode::base64::decode( m_stack.back().text().get());

                           if( range::size( binary) != range::size( value))
                              throw exception::casual::invalid::Node{ string::compose( "binary size missmatch - wanted: ", range::size( value), " got: ", range::size( binary))};

                           algorithm::copy( binary, std::begin( value));
                        }

                        pugi::xml_document m_document;
                        std::vector< pugi::xml_node> m_stack;

                     }; // Implementation
                  } // reader

                  namespace writer
                  {

                     class Implementation
                     {
                     public:

                        inline constexpr static auto archive_type() { return archive::Type::static_need_named;}

                        static decltype( auto) keys() { return local::keys();}


                        //! @param node Normally a pugi::xml_document
                        //!
                        //! @note Any possible document has to outlive the writer
                        explicit Implementation() : m_stack{ document()} {}

                        platform::size::type container_start( const platform::size::type size, const char* const name)
                        {
                           start( name);

                           auto element = m_stack.back();

                           // Stack 'em backwards
                           for( platform::size::type idx = 0; idx < size; ++idx)
                           {
                              m_stack.push_back( element.prepend_child( "element"));
                           }

                           return size;
                        }

                        void container_end( const char* const name)
                        {
                           end( name);
                        }

                        void composite_start( const char* const name)
                        {
                           start( name);
                        }

                        void composite_end(  const char* const name)
                        {
                           end( name);
                        }

                        template< typename T>
                        void write( const T& value, const char* const name)
                        {
                           start( name);
                           write( value);
                           end( name);
                        }

                        const pugi::xml_document& document() const { return m_document;}

                        void consume( std::ostream& xml)
                        {
                           m_document.save( xml, " ");
                           m_document.reset();
                        }

                     private:

                        void start( const char* const name)
                        {
                           if( name)
                           {
                              m_stack.push_back( m_stack.back().append_child( name));
                           }
                        }

                        void end( const char* const name)
                        {
                           m_stack.pop_back();
                        }

                        template<typename T>
                        void write( const T& value)
                        {
                           m_stack.back().text().set( std::to_string( value).c_str());
                        }

                        // A few overloads

                        void write( const bool& value)
                        {
                           std::ostringstream stream;
                           stream << std::boolalpha << value;
                           m_stack.back().text().set( stream.str().c_str());
                        }

                        void write( const char& value)
                        {
                           write( std::string{ value});
                        }

                        void write( const std::string& value)
                        {
                           m_stack.back().text().set( common::transcode::utf8::encode( value).c_str());
                        }

                        void write( const platform::binary::type& value)
                        {
                           write( view::binary::make( value));
                        }

                        void write( view::immutable::Binary value)
                        {
                           m_stack.back().text().set( common::transcode::base64::encode( value).c_str());
                        }

                        pugi::xml_document m_document;
                        std::vector< pugi::xml_node> m_stack;

                     }; // Implementation
                  } // writer
               } // <unnamed>
            } // local


            namespace strict
            {
               serialize::Reader reader( const std::string& source) { return create::reader::strict::create< local::reader::Implementation>( source);}
               serialize::Reader reader( std::istream& source) { return create::reader::strict::create< local::reader::Implementation>( source);}
               serialize::Reader reader( const platform::binary::type& source) { return create::reader::strict::create< local::reader::Implementation>( source);}
            } // strict

            namespace relaxed
            {    
               serialize::Reader reader( const std::string& source) { return create::reader::relaxed::create< local::reader::Implementation>( source);}
               serialize::Reader reader( std::istream& source) { return create::reader::relaxed::create< local::reader::Implementation>( source);}
               serialize::Reader reader( const platform::binary::type& source) { return create::reader::relaxed::create< local::reader::Implementation>( source);}
            } // relaxed

            namespace consumed
            {    
               serialize::Reader reader( const std::string& source) { return create::reader::consumed::create< local::reader::Implementation>( source);}
               serialize::Reader reader( std::istream& source) { return create::reader::consumed::create< local::reader::Implementation>( source);}
               serialize::Reader reader( const platform::binary::type& source) { return create::reader::consumed::create< local::reader::Implementation>( source);}
            } // consumed

            serialize::Writer writer()
            {
               return serialize::create::writer::create< local::writer::Implementation>();
            }

         } // xml

         namespace create
         {
            namespace reader
            {
               template struct Registration< xml::local::reader::Implementation>;
            } // writer
            namespace writer
            {
               template struct Registration< xml::local::writer::Implementation>;
            } // writer
         } // create

      } // serialize
   } // common

} // casual
