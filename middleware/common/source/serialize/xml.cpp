//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/serialize/xml.h"
#include "common/serialize/create.h"

#include "common/transcode.h"
#include "common/buffer/type.h"

#include "common/code/raise.h"
#include "common/code/casual.h"

#include "common/flag.h"

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

                  constexpr auto keys() 
                  {
                     using namespace std::string_view_literals; 
                     return array::make( "xml"sv, ".xml", common::buffer::type::xml);
                  }

                  namespace reader
                  {
                     namespace parse
                     {
                        constexpr auto directive = pugi::parse_escapes | pugi::parse_ws_pcdata_single;
                     } // parse

                     namespace empty
                     {
                        constexpr const auto document =  R"(<?xml version="1.0"?><value/>)";
                     } // empty

                     namespace load
                     {
                        void check( const pugi::xml_parse_result& result)
                        {
                           if( ! result) 
                              code::raise::error( code::casual::invalid_document, result.description());
                        }

                        pugi::xml_document document( std::istream& stream)
                        {
                           pugi::xml_document result;
                           check( result.load( stream, parse::directive));
                           return result;
                        }

                        pugi::xml_document document( const std::string& xml)
                        {
                           if( xml.empty())
                              return document( empty::document);

                           pugi::xml_document result;
                           check( result.load_buffer( xml.data(), xml.size(), parse::directive));
                           return result;
                        }

                        pugi::xml_document document( const platform::binary::type& xml)
                        {
                           if( xml.empty())
                              return document( empty::document);

                           pugi::xml_document result;
                           check( result.load_buffer( xml.data(), xml.size(), parse::directive));
                           return result;
                        }

                     } // load

                     namespace canonical
                     {
                        using Node = pugi::xml_node;

                        namespace filter
                        {
                           auto children( const Node& node)
                           {
                              auto filter_child = []( auto& child)
                              {
                                 return child.type() == pugi::xml_node_type::node_element;
                              };

                              return algorithm::filter( node.children(), filter_child);
                           }
                        } // filter

                        struct Parser 
                        {
                           auto operator() ( const Node& document)
                           {
                              Trace trace{ "xml::Parser::operator()"};

                              // take care of the document node
                              element( document);
                              
                              return std::exchange( m_canonical, {});
                           }

                        private:

                           static bool sequence( const Node& node)
                           {
                              return node.first_child().attribute( "sequence-value").as_bool();
                           }

                           static const char* name( const Node& node)
                           {
                              if( node.attribute( "sequence-value"))
                                 return nullptr;
                              return node.name();
                           }

                           void element( const Node& node)
                           {
                              if( auto children = filter::children( node))
                              {
                                 auto is_container = sequence( node);
                                 
                                 if( is_container)
                                    m_canonical.container_start( name( node));
                                 else
                                    m_canonical.composite_start( name( node));

                                 for( auto child : children)
                                    element( child);
                                 
                                 if( is_container)
                                    m_canonical.container_end();
                                 else
                                    m_canonical.composite_end();
                              }
                              else 
                              {
                                 // we only add if there is something in it
                                 if( ! node.text().empty())
                                    m_canonical.attribute( name( node));
                              }
                           }

                           policy::canonical::Representation m_canonical;
                        };

                        auto parse( const Node& document)
                        {
                           return Parser{}( document);
                        }

                     } // canonical

                     class Implementation
                     {
                     public:

                        constexpr static auto archive_properties() { return common::serialize::archive::Property::named;}

                        constexpr static auto keys() { return local::keys();}

                        //! @param node Normally a pugi::xml_document
                        //!
                        //! @note Any possible document has to outlive the reader
                        template< typename... Ts>
                        explicit Implementation( Ts&&... ts) : m_document{ load::document( std::forward< Ts>( ts)...)} 
                        {
                           m_stack.push_back( m_document);
                        }

                        std::tuple< platform::size::type, bool> container_start( const platform::size::type size, const char* name)
                        {
                           if( auto node = structured_node( name))
                           {
                              // Stack 'em backwards
                              auto content = node.children( "element");
                              for( auto child : range::reverse( content))
                              {
                                 // set attribute to help produce the canonical form.
                                 child.append_attribute( "sequence-value") = true;
                                 m_stack.push_back( std::move( child));
                              }

                              return std::make_tuple( std::distance( std::begin( content), std::end( content)), true);
                           }

                           return std::make_tuple( 0, false);
                        }

                        void container_end( const char* name)
                        {
                           m_stack.pop_back();
                        }

                        bool composite_start( const char* name)
                        {
                           return predicate::boolean( structured_node( name));
                        }

                        void composite_end( const char* name)
                        {
                           m_stack.pop_back();
                        }


                        template< typename T>
                        bool read( T& value, const char* name)
                        {
                           if( ! name)
                           {
                              // we assume it is a value from a previous pushed sequence node.
                              // and we consume it.
                              read( m_stack.back(), value);
                              m_stack.pop_back();

                              return true;
                           }

                           // it's a named child - does it exists?
                           if( auto node = m_stack.back().child( name))
                           {
                              read( node, value);
                              return true;
                           }
                           
                           return false;
                        }

                        policy::canonical::Representation canonical()
                        {
                           common::log::line( verbose::log, "stack.size(): ", m_stack.size());
                           return canonical::parse( m_document);
                        }

                     private:

                        pugi::xml_node structured_node( const char* name)
                        {
                           // if not named, we assume it is a value from a previous pushed sequence node, or root document
                           if( ! name)
                              return m_stack.back();

                           // it's a named child - does it exists?
                           if( auto node = m_stack.back().child( name))
                           {
                              // we push it to promote the node to 'current scope'. 
                              // composite_end/container_end will pop it.
                              m_stack.push_back( node);
                              return m_stack.back();
                           }

                           return {};
                        }

                        // Various stox-functions are more cumbersome to use if you
                        // wanna make sure the whole content is processed
                        static void read( const pugi::xml_node& node, bool& value)
                        {
                           const std::string_view boolean = node.text().get();
                           if( boolean == "true") 
                              value = true;
                           else if( boolean == "false") 
                              value = false;
                           else 
                              code::raise::error( code::casual::invalid_node, "unexpected type - node: ", node.name());
                        }

                        template<typename T>
                        static T extract( const pugi::xml_node& node)
                        {
                           std::istringstream stream( node.text().get());
                           T result;
                           stream >> result;
                           if( ! stream.fail() && stream.eof())
                              return result;

                           code::raise::error( code::casual::invalid_node, "unexpected type - node: ", node.name(), ", type: ", node.type());
                        }

                        static void read( const pugi::xml_node& node, short& value)
                        { value = extract< short>( node); }
                        static void read( const pugi::xml_node& node, int& value)
                        { value = extract< int>( node); }
                        static void read( const pugi::xml_node& node, long& value)
                        { value = extract< long>( node); }
                        static void read( const pugi::xml_node& node, long long& value)
                        { value = extract< long long>( node); }
                        static void read( const pugi::xml_node& node, float& value)
                        { value = extract< float>( node); }
                        static void read( const pugi::xml_node& node, double& value)
                        { value = extract< double>( node); }

                        static void read( const pugi::xml_node& node, char& value)
                        { 
                           // If empty string this should result in '\0'
                           value = *transcode::utf8::string::decode( node.text().get()).data();
                        }
                        static void read( const pugi::xml_node& node, std::string& value)
                        {
                           value = transcode::utf8::string::decode( node.text().get());
                        }
                        static void read( const pugi::xml_node& node, std::u8string& value)
                        {
                           value = transcode::utf8::cast( node.text().get());
                        }
                        static void read( const pugi::xml_node& node, platform::binary::type& value)
                        { 
                           value = transcode::base64::decode( node.text().get()); 
                        }
                        static void read( const pugi::xml_node& node, view::Binary value)
                        { 
                           auto binary = transcode::base64::decode( node.text().get());

                           if( range::size( binary) != range::size( value))
                              code::raise::error( code::casual::invalid_node, "binary size mismatch - wanted: ", range::size( value), " got: ", range::size( binary));

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

                        constexpr static auto archive_properties() { return common::serialize::archive::Property::named;}

                        constexpr static auto keys() { return local::keys();}


                        //! @param node Normally a pugi::xml_document
                        //!
                        //! @note Any possible document has to outlive the writer
                        explicit Implementation() : m_stack{ document()} {}

                        platform::size::type container_start( const platform::size::type size, const char* name)
                        {
                           start( name);

                           auto element = m_stack.back();

                           // Stack 'em backwards
                           for( platform::size::type idx = 0; idx < size; ++idx)
                              m_stack.push_back( element.prepend_child( "element"));

                           return size;
                        }

                        void container_end( const char* name)
                        {
                           end( name);
                        }

                        void composite_start( const char* name)
                        {
                           start( name);
                        }

                        void composite_end(  const char* name)
                        {
                           end( name);
                        }

                        template< typename T>
                        void write( const T& value, const char* name)
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

                        void start( const char* name)
                        {
                           if( name)
                           {
                              m_stack.push_back( m_stack.back().append_child( name));
                           }
                        }

                        void end( const char* name)
                        {
                           m_stack.pop_back();
                        }

                        template<typename T>
                        void write( const T& value)
                        {
                           set( std::to_string( value));
                        }

                        // A few overloads

                        void write( const bool& value)
                        {
                           std::ostringstream stream;
                           stream << std::boolalpha << value;
                           set( std::move( stream).str());
                        }

                        void write( const char& value)
                        {
                           write( std::string{ value});
                        }

                        void write( const std::string& value)
                        {
                           set( transcode::utf8::string::encode( value));
                        }

                        void write( const std::u8string& value)
                        {
                           set( transcode::utf8::cast( value));
                        }

                        void write( const platform::binary::type& value)
                        {
                           write( view::binary::make( value));
                        }

                        void write( view::immutable::Binary value)
                        {
                           set( transcode::base64::encode( value));
                        }


                        void set(const auto& value)
                        {
                           m_stack.back().text().set( value.data(), value.size());
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
