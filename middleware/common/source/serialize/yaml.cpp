//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/serialize/yaml.h"

#include "common/serialize/policy.h"
#include "common/serialize/create.h"

#include "common/code/raise.h"
#include "common/code/casual.h"

#include "common/transcode.h"
#include "common/buffer/type.h"

#include <yaml-cpp/yaml.h>
#include <yaml-cpp/binary.h>

namespace casual
{
   namespace common::serialize
   {
      namespace yaml
      {
         namespace local
         {
            namespace
            {
               constexpr auto keys() 
               {
                  using namespace std::string_view_literals; 
                  return array::make( "yaml"sv, ".yaml"sv, "yml"sv, ".yml"sv, buffer::type::yaml);
               };

               namespace reader
               {
                  namespace load
                  {
                     YAML::Node document( std::istream& stream)
                     {
                        try
                        {
                           auto result = YAML::Load( stream);

                           if( result)
                              return result;
                           else
                              code::raise::error( code::casual::invalid_document, "no document");
                        }
                        catch( const YAML::ParserException& e)
                        {
                           code::raise::error( code::casual::invalid_document, e.what());
                        }
                     }

                     YAML::Node document( const std::string& yaml)
                     {
                        // we need a real document
                        std::istringstream stream{ yaml.empty() ? "---\n" : yaml};
                        return document( stream);
                     }

                     YAML::Node document( const platform::binary::type& yaml)
                     {
                        return document( std::string( yaml.data(), yaml.size()));
                     }

                  } // load

                  namespace canonical
                  {
                     struct Parser 
                     {
                        auto operator() ( const YAML::Node& document)
                        {
                           deduce( document, nullptr);
                           return std::exchange( m_canonical, {});
                        }

                     private:

                        void deduce( const YAML::Node& node, const char* name)
                        {
                           switch( node.Type())
                           { 
                              case YAML::NodeType::Undefined: break; // ?!?
                              case YAML::NodeType::Null: break; // ?!?
                              case YAML::NodeType::Scalar: scalar( node, name); break;
                              case YAML::NodeType::Sequence: sequence( node, name); break;
                              case YAML::NodeType::Map: map( node, name); break;
                           }
                        }

                        void scalar( const YAML::Node& node, const char* name)
                        {
                           m_canonical.attribute( name);
                        }

                        void sequence( const YAML::Node& node, const char* name)
                        {
                           m_canonical.container_start( name);

                           for( auto& current : node)
                              deduce( current, nullptr);
                           
                           m_canonical.container_end();
                        }

                        void map( const YAML::Node& node, const char* name)
                        {
                           m_canonical.composite_start( name);

                           for( auto current = node.begin(); current != node.end(); ++current)
                              deduce( current->second, current->first.as<std::string>().data());
                           
                           m_canonical.composite_end();
                        }

                        policy::canonical::Representation m_canonical;
                     };

                     auto parse( const YAML::Node& document)
                     {
                        return Parser{}( document);
                     }
                     
                  } // canonical

                  class Implementation
                  {
                  public:

                     constexpr static auto archive_type() { return archive::Type::static_need_named;}

                     constexpr static auto keys() { return local::keys();}

                     template< typename... Ts>
                     Implementation( Ts&&... ts) : m_stack{ load::document( std::forward< Ts>( ts)...)}, m_document{ range::back( m_stack)} {}

                     std::tuple< platform::size::type, bool> container_start( platform::size::type size, const char* name)
                     {
                        auto node = structured_node( name);

                        if( ! node)
                           return std::make_tuple( 0, false);

                        size = node.size();

                        if( size)
                        {
                           // If there are elements, it must be a sequence
                           if( node.Type() != YAML::NodeType::Sequence)
                              code::raise::error( code::casual::invalid_document, "expected sequence for node: ", name);

                           // We stack'em in reverse order
                           for( auto index = size; index > 0; --index)
                              m_stack.push_back( node[ index - 1]);
                        }

                        return std::make_tuple( size, true);
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

                        if( name)
                        {
                           // has name == has to exist in a map
                           if( auto node = current_map_node( name))
                           {
                              read( node, value);
                              return true;
                           }

                           return false;
                        }

                        assert( ! m_stack.empty());
            
                        // we assume it is a value from a previous pushed sequence node.
                        // and we consume it.
                        read( m_stack.back(), value);
                        m_stack.pop_back();
                        
                        return true;
                     }

                     policy::canonical::Representation canonical()
                     {
                        return canonical::parse( m_document);
                     }

                  private:

                     YAML::Node current_map_node( const char* name)
                     {
                        assert( name);
                        assert( ! m_stack.empty());

                        // the back node should be a "map"
                        if( m_stack.back().Type() != YAML::NodeType::Map)
                           code::raise::error( code::casual::invalid_document, "expected map for '", name, "'");

                        return m_stack.back()[ name];
                     }

                     YAML::Node structured_node( const char* name)
                     {
                        // if not named, we assume it is a value from a previous pushed sequence node, or root document
                        if( ! name)
                        {
                           assert( ! m_stack.empty());
                           return m_stack.back();
                        }

                        if( auto node = current_map_node( name))
                        {
                           // we push it to promote the node to 'current scope'. 
                           // composite_end/container_end will pop it.
                           m_stack.push_back( node);
                           return m_stack.back();
                        }

                        // 'nil' value. default ctor sets an "ok state", end operator bool returns true, 
                        // not intuitive...
                        return YAML::Node{ YAML::NodeType::Undefined};
                     }

                     template<typename T>
                     static void consume( const YAML::Node& node, T& value)
                     {
                        try
                        {
                           // node can be null, if no value is set to the node.
                           if( ! node.IsNull())
                              value = node.as<T>();
                        }
                        catch( const YAML::InvalidScalar& e)
                        {
                           code::raise::error( code::casual::invalid_node, e.what());
                        }
                     }

                     static void read( const YAML::Node& node, bool& value) { consume( node, value);}
                     static void read( const YAML::Node& node, short& value) { consume( node, value);}
                     static void read( const YAML::Node& node, long& value) { consume( node, value);}
                     static void read( const YAML::Node& node, long long& value) { consume( node, value);}
                     static void read( const YAML::Node& node, float& value) { consume( node, value);}
                     static void read( const YAML::Node& node, double& value) { consume( node, value);}
                     static void read( const YAML::Node& node, char& value)
                     {
                        std::string string;
                        consume( node, string);
                        value = *transcode::utf8::string::decode( string).data();
                     }
                     static void read( const YAML::Node& node, std::string& value)
                     {
                        consume( node, value);
                        value = transcode::utf8::string::decode( value);
                     }
                     static void read( const YAML::Node& node, std::u8string& value)
                     {
                        std::string string;
                        consume( node, string);
                        value = transcode::utf8::encode( string);
                     }
                     static void read( const YAML::Node& node, platform::binary::type& value)
                     {
                        YAML::Binary binary;
                        consume( node, binary);
                        value.assign( binary.data(), binary.data() + binary.size());
                     }
                     static void read( const YAML::Node& node, view::Binary value)
                     {
                        YAML::Binary binary;
                        consume( node, binary);
                        algorithm::copy( range::make( binary.data(), binary.size()), value);
                     }

                  protected:
                     std::vector< YAML::Node> m_stack;
                     YAML::Node m_document;
                  };

               } // reader

               namespace writer
               {

                  class Implementation
                  {
                  public:

                     enum class State : short
                     {
                        empty,
                        implicit_map,
                        unnamed_root,
                     };

                     inline constexpr static auto archive_type() { return archive::Type::static_need_named;}

                     static decltype( auto) keys() { return local::keys();}

                     Implementation()
                     {
                        m_output.SetFloatPrecision( std::numeric_limits< float >::max_digits10);
                        m_output.SetDoublePrecision( std::numeric_limits< double >::max_digits10);
                        m_output << YAML::BeginDoc;
                     }

                     platform::size::type container_start( const platform::size::type size, const char* name)
                     {
                        possible_implicit_map( name);

                        m_output << YAML::BeginSeq;

                        return size;
                     }

                     void container_end( const char*)
                     {
                        m_output << YAML::EndSeq;
                     }

                     void composite_start( const char* name)
                     {
                        possible_implicit_map( name);
                           
                        m_output << YAML::BeginMap;
                     }

                     void composite_end( const char*)
                     {
                        m_output << YAML::EndMap;
                     }

                     template< typename T>
                     void write( const T& value, const char* name)
                     {
                        possible_implicit_map( name);
                        write( value);
                     }

                     const YAML::Emitter& document() const { return m_output;}

                     auto consume()
                     {
                        if( std::exchange( m_state, State::empty) == State::implicit_map)
                           m_output << YAML::EndMap;

                        m_output << YAML::EndDoc;

                        auto size = m_output.size();
                        auto offset = std::exchange( m_offset, size);

                        m_output << YAML::BeginDoc;

                        return range::make( m_output.c_str() + offset, size - offset);
                     }

                  private:

                     void possible_implicit_map( const char* key)
                     {
                        if( key)
                        {
                           if( m_state == State::empty)
                           {
                              // we need to wrap in a map
                              m_output << YAML::BeginMap;
                              m_state = State::implicit_map;
                           }
                           
                           m_output << YAML::Key << key;
                           m_output << YAML::Value;
                        }  
                        else if( m_state == State::empty)
                           m_state = State::unnamed_root;
                     }


                     template< typename T>
                     void write( const T& value)
                     {
                        m_output << value;
                     }

                     // A few overloads

                     void write( const char& value)
                     {
                        m_output << YAML::SingleQuoted << transcode::utf8::string::encode( std::string{ value});
                     }

                     void write( const std::string& value)
                     {
                        m_output << YAML::DoubleQuoted << transcode::utf8::string::encode( value);
                     }

                     void write( const std::u8string& value)
                     {
                        m_output << YAML::DoubleQuoted << std::string{ transcode::utf8::cast( value)};
                     }

                     void write( const platform::binary::type& value)
                     {
                        write( view::binary::make( value));
                     }

                     void write( view::immutable::Binary value)
                     {
                        // TODO: Is this conformant ?
                        const YAML::Binary binary( reinterpret_cast< const unsigned char*>( value.data()), value.size());
                        m_output << binary;
                     }

                     State m_state = State::empty;
                     YAML::Emitter m_output;
                     platform::size::type m_offset = 0;
                  };

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
         }

         namespace consumed
         {    
            serialize::Reader reader( const std::string& source) { return create::reader::consumed::create< local::reader::Implementation>( source);}
            serialize::Reader reader( std::istream& source) { return create::reader::consumed::create< local::reader::Implementation>( source);}
            serialize::Reader reader( const platform::binary::type& source) { return create::reader::consumed::create< local::reader::Implementation>( source);}
         }

         serialize::Writer writer()
         {
            return serialize::create::writer::create< local::writer::Implementation>();
         }

      } // yaml

      namespace create
      {
         namespace reader
         {
            template struct Registration< yaml::local::reader::Implementation>;
         } // writer
         namespace writer
         {
            template struct Registration< yaml::local::writer::Implementation>;
         } // writer
      } // create
      
   } // common::serialize
} // casual

