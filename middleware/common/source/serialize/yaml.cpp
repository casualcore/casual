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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <yaml-cpp/yaml.h>
#include <yaml-cpp/binary.h>
#include <yaml-cpp/eventhandler.h>
#include <yaml-cpp/anchor.h>
#pragma GCC diagnostic pop


namespace casual
{
   namespace common
   {
      namespace serialize
      {
         namespace yaml
         {
            namespace local
            {
               namespace
               {
                  std::vector< std::string> keys() { return { "yaml", "yml", common::buffer::type::yaml()};};

                  namespace reader
                  {

                     struct Load
                     {

                        const YAML::Node& operator() ( YAML::Node& document, std::istream& stream)
                        {
                           try
                           {
                              YAML::Parser parser( stream);
                              if( ! parser.GetNextDocument( document))
                                 code::raise::error( code::casual::invalid_document, "no document");
                           }
                           catch( const YAML::ParserException& e)
                           {
                              code::raise::error( code::casual::invalid_document, e.what());
                           }

                           return document;
                        }

                        const YAML::Node& operator() ( YAML::Node& document, const platform::binary::type& yaml)
                        {
                           return operator() ( document, yaml.data(), yaml.size());
                        }

                        const YAML::Node& operator() ( YAML::Node& document, const std::string& yaml)
                        {
                           std::istringstream stream{ wrap_empty( yaml)};
                           return operator()( document, stream);
                        }

                        const YAML::Node& operator() ( YAML::Node& document, const char* const yaml, const platform::size::type size)
                        {
                           std::istringstream stream{ wrap_empty( std::string( yaml, size))};
                           return operator()( document, stream);
                        }
                     private:
                        const std::string& empty()
                        {
                           static const std::string document{ "---\n"};
                           return document;
                        }

                        const std::string& wrap_empty( const std::string& yaml)
                        {
                           if( yaml.empty())
                              return empty();

                           return yaml;
                        }

                        const char* wrap_empty( const char* const yaml)
                        {
                           if( ! yaml || yaml[ 0] == '\0')
                              return empty().c_str();

                           return yaml;
                        }
                     };

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
                                 case YAML::NodeType::Scalar: scalar( node, name); break;
                                 case YAML::NodeType::Sequence: sequence( node, name); break;
                                 case YAML::NodeType::Map: map( node, name); break;
                                 case YAML::NodeType::Null: /*???*/ break;
                              }
                           }

                           void scalar( const YAML::Node& node, const char* name)
                           {
                              m_canonical.attribute( name);
                           }

                           void sequence( const YAML::Node& node, const char* name)
                           {
                              start( name);

                              for( auto& current : node)
                                 deduce( current, "element");  
                              
                              end( name);
                           }

                           void map( const YAML::Node& node, const char* name)
                           {
                              start( name);

                              for( auto current = node.begin(); current != node.end(); ++current)
                              {
                                 std::string key;
                                 current.first() >> key;
                                 deduce( current.second(), key.data());
                              }
                              
                              end( name);
                           }

                           void start( const char* name)
                           {
                              // take care of the first node which doesn't have a name, and is
                              // not a composite in an archive sense.
                              if( name) 
                                 m_canonical.composite_start( name);
                           }

                           void end( const char* name)
                           {
                              // take care of the first node which doesn't have a name, and is
                              // not a composite in an archive sense.
                              if( name)
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

                        inline constexpr static auto archive_type() { return archive::Type::static_need_named;}

                        static decltype( auto) keys() { return local::keys();}

                        template< typename... Ts>
                        Implementation( Ts&&... ts) : m_stack{ &Load{}( m_document, std::forward< Ts>( ts)...)} {}

                        std::tuple< platform::size::type, bool> container_start( platform::size::type size, const char* const name)
                        {
                           if( ! start( name))
                              return std::make_tuple( 0, false);

                           const auto& node = *m_stack.back();

                           size = node.size();

                           if( size)
                           {
                              // If there are elements, it must be a sequence
                              if( node.Type() != YAML::NodeType::Sequence)
                                 code::raise::error( code::casual::invalid_document, "expected sequence");

                              // We stack'em in reverse order
                              for( auto index = size; index > 0; --index)
                                 m_stack.push_back( &node[ index - 1]);
                           }

                           return std::make_tuple( size, true);

                        }

                        void container_end( const char* const name)
                        {
                           end( name);
                        }

                        bool composite_start( const char* const name)
                        {
                           if( ! start( name))
                              return false;

                           if( m_stack.back()->Type() != YAML::NodeType::Map)
                              code::raise::error( code::casual::invalid_document, "expected map");

                           return true;
                        }

                        void composite_end(  const char* const name)
                        {
                           end( name);
                        }

                        template< typename T>
                        bool read( T& value, const char* const name)
                        {
                           if( start( name))
                           {
                              if( m_stack.back()->Type() != YAML::NodeType::Null)
                                 read( value);

                              end( name);
                              return true;
                           }

                           return false;
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
                              auto node = m_stack.back()->FindValue( name);

                              if( node)
                                 m_stack.push_back( node);
                              else
                                 return false;
                           }

                           // Either we found the node or we assume it's an 'unnamed' container
                           // element that is already pushed to the stack
                           return true;
                        }

                        void end( const char* const name)
                        {
                           m_stack.pop_back();
                        }

                        template<typename T>
                        void consume( const YAML::Node& node, T& value) const
                        {
                           try
                           {
                              node >> value;
                           }
                           catch( const YAML::InvalidScalar& e)
                           {
                              code::raise::error( code::casual::invalid_node, e.what());
                           }
                        }

                        void read( bool& value) const { consume( *m_stack.back(), value);}
                        void read( short& value) const { consume( *m_stack.back(), value);}
                        void read( long& value) const { consume( *m_stack.back(), value);}
                        void read( long long& value) const { consume( *m_stack.back(), value);}
                        void read( float& value) const { consume( *m_stack.back(), value);}
                        void read( double& value) const { consume( *m_stack.back(), value);}
                        void read( char& value) const
                        {
                           consume( *m_stack.back(), value);
                           value = *common::transcode::utf8::decode( { value}).c_str();
                        }
                        void read( std::string& value) const
                        {
                           consume( *m_stack.back(), value);
                           value = common::transcode::utf8::decode( value);
                        }
                        void read( platform::binary::type& value) const
                        {
                           YAML::Binary binary;
                           consume( *m_stack.back(), binary);
                           value.assign( binary.data(), binary.data() + binary.size());
                        }

                        void read( view::Binary value) const
                        {
                           YAML::Binary binary;
                           consume( *m_stack.back(), binary);
                           algorithm::copy( range::make( binary.data(), binary.size()), value);
                        }

                     protected:
                        YAML::Node m_document;
                        std::vector< const YAML::Node*> m_stack;

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

                        platform::size::type container_start( const platform::size::type size, const char* const name)
                        {
                           possible_implicit_map( name);

                           m_output << YAML::BeginSeq;

                           return size;
                        }

                        void container_end( const char* const name)
                        {
                           m_output << YAML::EndSeq;
                        }

                        void composite_start( const char* const name)
                        {
                           possible_implicit_map( name);
                              
                           m_output << YAML::BeginMap;
                        }

                        void composite_end(  const char* const name)
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

                           return common::range::make( m_output.c_str() + offset, size - offset);
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
                           m_output << common::transcode::utf8::encode( { value});
                        }

                        void write( const std::string& value)
                        {
                           m_output << YAML::DoubleQuoted << common::transcode::utf8::encode( value);
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
         
      } // serialize
   } // common
} // casual

