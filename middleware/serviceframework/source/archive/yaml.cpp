//!
//! casual
//!

#include "sf/archive/yaml.h"

#include "sf/exception.h"
#include "sf/log.h"

#include "common/transcode.h"


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
                  const std::string& empty()
                  {
                     static const std::string document{ "---\n"};
                     return document;
                  }

                  const std::string& stream( const std::string& yaml)
                  {
                     if( yaml.empty())
                     {
                        return empty();
                     }
                     return yaml;
                  }

                  const char* stream( const char* const yaml)
                  {
                     if( ! yaml || yaml[ 0] == '\0')
                     {
                        return empty().c_str();
                     }
                     return yaml;
                  }

               } // <unnamed>
            } // local

            Load::Load() = default;
            Load::~Load() = default;

            const YAML::Node& Load::operator() () const noexcept
            {
               return m_document;
            }


            const YAML::Node& Load::operator() ( std::istream& stream)
            {
               try
               {
                  YAML::Parser parser( stream);
                  if( ! parser.GetNextDocument( m_document))
                  {
                     throw exception::archive::invalid::Document{ "no document"};
                  }
               }
               catch( const YAML::ParserException& e)
               {
                  throw exception::archive::invalid::Document{ e.what()};
               }

               return m_document;
            }

            const YAML::Node& Load::operator() ( const platform::binary::type& yaml)
            {
               return operator() ( yaml.data(), yaml.size());
            }

            const YAML::Node& Load::operator() ( const std::string& yaml)
            {
               std::istringstream stream{ local::stream( yaml)};
               return (*this)( stream);
            }

            const YAML::Node& Load::operator() ( const char* const yaml, const platform::size::type size)
            {
               std::istringstream stream{ local::stream( std::string( yaml, size))};
               return (*this)( stream);
            }


            const YAML::Node& Load::operator() ( const char* const yaml)
            {
               std::istringstream stream{ local::stream( yaml)};
               return (*this)( stream);
            }

            namespace reader
            {
               Implementation::Implementation( const YAML::Node& node) : m_stack{ &node} {}

               std::tuple< platform::size::type, bool> Implementation::container_start( platform::size::type size, const char* const name)
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

               void Implementation::container_end( const char* const name)
               {
                  end( name);
               }

               bool Implementation::serialtype_start( const char* const name)
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

               void Implementation::serialtype_end( const char* const name)
               {
                  end( name);
               }

               bool Implementation::start( const char* const name)
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

               void Implementation::end( const char* const name)
               {
                  m_stack.pop_back();
               }

               namespace
               {
                  namespace local
                  {
                     template<typename T>
                     void read( const YAML::Node& node, T& value)
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
                  }
               }


               void Implementation::read( bool& value) const
               { local::read( *m_stack.back(), value);}
               void Implementation::read( short& value) const
               { local::read( *m_stack.back(), value);}
               void Implementation::read( long& value) const
               { local::read( *m_stack.back(), value);}
               void Implementation::read( long long& value) const
               { local::read( *m_stack.back(), value);}
               void Implementation::read( float& value) const
               { local::read( *m_stack.back(), value);}
               void Implementation::read( double& value) const
               { local::read( *m_stack.back(), value);}
               void Implementation::read( char& value) const
               {
                  local::read( *m_stack.back(), value);
                  value = *common::transcode::utf8::decode( { value}).c_str();
               }
               void Implementation::read( std::string& value) const
               {
                  local::read( *m_stack.back(), value);
                  value = common::transcode::utf8::decode( value);
               }
               void Implementation::read( platform::binary::type& value) const
               {
                  YAML::Binary binary;
                  local::read( *m_stack.back(), binary);
                  value.assign( binary.data(), binary.data() + binary.size());
               }


            } // reader

            Save::Save() = default;
            Save::~Save() = default;

            YAML::Emitter& Save::operator() () noexcept
            {
               return m_emitter;
            }
            void Save::operator() ( std::ostream& yaml) const
            {
               yaml << m_emitter.c_str();
            }

            void Save::operator() ( platform::binary::type& yaml) const
            {
               yaml.resize( m_emitter.size());
               common::algorithm::copy(
                     common::range::make( m_emitter.c_str(), m_emitter.size()),
                     std::begin( yaml));
            }
            void Save::operator() ( std::string& yaml) const
            {
               yaml = m_emitter.c_str();
            }

            namespace writer
            {

               Implementation::Implementation( YAML::Emitter& output) : m_output( output)
               {
                  m_output << YAML::BeginDoc;
                  m_output << YAML::BeginMap;
               }


               platform::size::type Implementation::container_start( const platform::size::type size, const char* const name)
               {
                  if( name)
                  {
                     m_output << YAML::Key << name;
                     m_output << YAML::Value;
                  }
                  m_output << YAML::BeginSeq;

                  return size;
               }

               void Implementation::container_end( const char* const name)
               {
                  m_output << YAML::EndSeq;
                  m_output << YAML::Newline;
               }

               void Implementation::serialtype_start( const char* const name)
               {
                  if( name)
                  {
                     m_output << YAML::Key << name;
                     m_output << YAML::Value;
                  }
                  m_output << YAML::BeginMap;
               }

               void Implementation::serialtype_end( const char* const name)
               {
                  m_output << YAML::EndMap;
                  m_output << YAML::Newline;
               }

               void Implementation::write( const char& value)
               {
                  m_output << common::transcode::utf8::encode( { value});
               }

               void Implementation::write( const std::string& value)
               {
                  m_output << common::transcode::utf8::encode( value);
               }

               void Implementation::write( const platform::binary::type& value)
               {
                  // TODO: Is this conformant ?
                  const YAML::Binary binary{ reinterpret_cast< const unsigned char*>( value.data()), value.size()};
                  m_output << binary;
               }


            } // writer

         } // yaml
      } // archive
   } // sf
} // casual

