//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "sf/archive/yaml.h"

#include "sf/archive/policy.h"

#include "sf/exception.h"
#include "sf/log.h"

#include "common/transcode.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <yaml-cpp/yaml.h>
#include <yaml-cpp/binary.h>
#include <yaml-cpp/eventhandler.h>
#include <yaml-cpp/anchor.h>
#pragma GCC diagnostic pop


namespace casual
{
   namespace sf
   {
      namespace archive
      {
         namespace yaml
         {
            namespace local
            {
               namespace
               {
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
                              {
                                 throw exception::archive::invalid::Document{ "no document"};
                              }
                           }
                           catch( const YAML::ParserException& e)
                           {
                              throw exception::archive::invalid::Document{ e.what()};
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
                           {
                              return empty();
                           }
                           return yaml;
                        }

                        const char* wrap_empty( const char* const yaml)
                        {
                           if( ! yaml || yaml[ 0] == '\0')
                           {
                              return empty().c_str();
                           }
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

                              for( auto current = node.begin(); current != node.end(); ++current)
                              {
                                 deduce( *current, "element");  
                              }
                              
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

                        template< typename... Ts>
                        Implementation( Ts&&... ts) : m_stack{ &Load{}( m_document, std::forward< Ts>( ts)...)} {}

                        std::tuple< platform::size::type, bool> container_start( platform::size::type size, const char* const name)
                        {
                           if( ! start( name))
                           {
                              return std::make_tuple( 0, false);
                           }

                           const auto& node = *m_stack.back();

                           size = node.size();

                           if( size)
                           {
                              //
                              // If there are elements, it must be a sequence
                              //

                              if( node.Type() != YAML::NodeType::Sequence)
                              {
                                 throw exception::archive::invalid::Node{ "expected sequence"};
                              }

                              //
                              // We stack'em in reverse order
                              //

                              for( auto index = size; index > 0; --index)
                              {
                                 m_stack.push_back( &node[ index - 1]);
                              }
                           }

                           return std::make_tuple( size, true);

                        }

                        void container_end( const char* const name)
                        {
                           end( name);
                        }

                        bool serialtype_start( const char* const name)
                        {
                           if( ! start( name))
                           {
                              return false;
                           }

                           if( m_stack.back()->Type() != YAML::NodeType::Map)
                           {
                              throw exception::archive::invalid::Node{ "expected map"};
                           }

                           return true;
                        }

                        void serialtype_end( const char* const name)
                        {
                           end( name);
                        }

                        template< typename T>
                        bool read( T& value, const char* const name)
                        {
                           if( start( name))
                           {
                              if( m_stack.back()->Type() != YAML::NodeType::Null)
                              {
                                 read( value);
                              }

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
                              {
                                 m_stack.push_back( node);
                              }
                              else
                              {
                                 return false;
                              }
                           }

                           //
                           // Either we found the node or we assume it's an 'unnamed' container
                           // element that is already pushed to the stack
                           //

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
                              throw exception::archive::invalid::Node{ e.what()};
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

                     protected:
                        YAML::Node m_document;
                        std::vector< const YAML::Node*> m_stack;

                     };

                     namespace consumed
                     {
                        template< typename T>
                        auto create( T&& source)
                        {
                           return archive::Reader::emplace< archive::policy::Consumed< Implementation>>( std::forward< T>( source));
                        }

                     } // consumed

                     namespace strict
                     {
                        template< typename T>
                        auto create( T&& source)
                        {
                           return archive::Reader::emplace< archive::policy::Strict< Implementation>>( std::forward< T>( source));
                        }
                     } // strict

                     namespace relaxed
                     {
                        template< typename T>
                        auto create( T&& source)
                        {
                           return archive::Reader::emplace< archive::policy::Relaxed< Implementation>>( std::forward< T>( source));
                        }
                     } // relaxed

                  } // reader


                  namespace writer
                  {

                     class Implementation
                     {

                     public:

                        using buffer_type = YAML::Emitter;

                        Implementation()
                        {
                           m_output << YAML::BeginDoc;
                           m_output << YAML::BeginMap;
                        }


                        platform::size::type container_start( const platform::size::type size, const char* const name)
                        {
                           if( name)
                           {
                              m_output << YAML::Key << name;
                              m_output << YAML::Value;
                           }
                           m_output << YAML::BeginSeq;

                           return size;
                        }

                        void container_end( const char* const name)
                        {
                           m_output << YAML::EndSeq;
                           m_output << YAML::Newline;
                        }

                        void serialtype_start( const char* const name)
                        {
                           if( name)
                           {
                              m_output << YAML::Key << name;
                              m_output << YAML::Value;
                           }
                           m_output << YAML::BeginMap;
                        }

                        void serialtype_end( const char* const name)
                        {
                           m_output << YAML::EndMap;
                           m_output << YAML::Newline;
                        }

                        template< typename T>
                        void write( const T& value, const char* name)
                        {
                           if( name)
                           {
                              m_output << YAML::Key << name;
                              m_output << YAML::Value;
                           }

                           write( value);
                        }

                        const YAML::Emitter& document() const { return m_output;}

                     private:

                        template< typename T>
                        void write( const T& value)
                        {
                           m_output << value;
                        }

                        //
                        // A few overloads
                        //

                        void write( const char& value)
                        {
                           m_output << common::transcode::utf8::encode( { value});
                        }

                        void write( const std::string& value)
                        {
                           m_output << common::transcode::utf8::encode( value);
                        }

                        void write( const platform::binary::type& value)
                        {
                           // TODO: Is this conformant ?
                           const YAML::Binary binary{ reinterpret_cast< const unsigned char*>( value.data()), value.size()};
                           m_output << binary;
                        }

                     
                        YAML::Emitter m_output;
                     };

                     void write_document( const YAML::Emitter& document, std::ostream& yaml)
                     {
                        yaml << document.c_str();
                     }

                     void write_document( const YAML::Emitter& document, platform::binary::type& yaml)
                     {
                        yaml.resize( document.size());
                        common::algorithm::copy(
                              common::range::make( document.c_str(), document.size()),
                              std::begin( yaml));
                     }
                     void write_document( const YAML::Emitter& document, std::string& yaml)
                     {
                        yaml = document.c_str();
                     }

                     template< typename Out> 
                     struct Holder : Implementation
                     {
                        Holder( Out& out) : m_out( out) {}

                        void flush() 
                        {
                           write_document( Implementation::document(), m_out.get());
                        }

                        std::reference_wrapper< Out> m_out;
                     };
                  } // writer

               } // <unnamed>
            } // local

            archive::Reader reader( const std::string& source) { return local::reader::strict::create( source);}
            archive::Reader reader( std::istream& source) { return local::reader::strict::create( source);}
            archive::Reader reader( const common::platform::binary::type& source) { return local::reader::strict::create( source);}

            namespace relaxed
            {    
               archive::Reader reader( const std::string& source) { return local::reader::relaxed::create( source);}
               archive::Reader reader( std::istream& source) { return local::reader::relaxed::create( source);}
               archive::Reader reader( const common::platform::binary::type& source) { return local::reader::relaxed::create( source);}
            }

            namespace consumed
            {    
               archive::Reader reader( const std::string& source) { return local::reader::consumed::create( source);}
               archive::Reader reader( std::istream& source) { return local::reader::consumed::create( source);}
               archive::Reader reader( const common::platform::binary::type& source) { return local::reader::consumed::create( source);}
            }

            archive::Writer writer( std::string& destination)
            {
               return archive::Writer::emplace< local::writer::Holder< std::string>>( destination);
            }

            archive::Writer writer( std::ostream& destination)
            {
               return archive::Writer::emplace< local::writer::Holder< std::ostream>>( destination);
            }

            archive::Writer writer( common::platform::binary::type& destination)
            {
               return archive::Writer::emplace< local::writer::Holder< common::platform::binary::type>>( destination);
            }

         } // yaml
      } // archive
   } // sf
} // casual

