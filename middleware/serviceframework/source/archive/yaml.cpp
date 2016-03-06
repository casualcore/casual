/*
 * yaml.cpp
 *
 *  Created on: Jan 23, 2015
 *      Author: kristone
 */

#include "sf/archive/yaml.h"

#include "sf/exception.h"

#include "common/transcode.h"


namespace casual
{
   namespace sf
   {
      namespace archive
      {
         namespace yaml
         {

            Load::Load() = default;
            Load::~Load() = default;

            const YAML::Node& Load::serialize( std::istream& stream)
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

               return source();
            }

            const YAML::Node& Load::serialize( const std::string& yaml)
            {
               std::istringstream stream( yaml);
               return serialize( stream);
            }

            const YAML::Node& Load::serialize( const char* const yaml)
            {
               std::istringstream stream( yaml);
               return serialize( stream);
            }

            const YAML::Node& Load::source() const
            {
               return m_document;
            }

            namespace reader
            {
               Implementation::Implementation( const YAML::Node& node) : m_stack{ &node} {}

               std::tuple< std::size_t, bool> Implementation::container_start( std::size_t size, const char* const name)
               {
                  if( ! start( name))
                  {
                     return std::make_tuple( 0, false);
                  }

                  const auto& node = *m_stack.back();

                  if( m_stack.back()->Type() != YAML::NodeType::Sequence)
                  {
                     throw exception::archive::invalid::Node{ "expected sequence"};
                  }

                  //
                  // We stack 'em in reverse order
                  //

                  size = node.size();

                  for( auto index = size; index > 0; --index)
                  {
                     m_stack.push_back( &node[ index - 1]);
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
                     //
                     // TODO: (Maybe) remove this check whenever archive is fixed
                     //
                     if( m_stack.back())
                     {
                        m_stack.push_back( m_stack.back()->FindValue( name));
                     }
                     else
                     {
                        m_stack.push_back( nullptr);
                     }
                  }

                  return m_stack.back() != nullptr;

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
               void Implementation::read( platform::binary_type& value) const
               {
                  YAML::Binary binary;
                  local::read( *m_stack.back(), binary);
                  value.assign( binary.data(), binary.data() + binary.size());
               }


            } // reader

            Save::Save() = default;
            Save::~Save() = default;

            void Save::serialize( std::ostream& stream) const
            {
               stream << m_emitter.c_str();
            }

            void Save::serialize( std::string& yaml) const
            {
               yaml = m_emitter.c_str();
            }

            YAML::Emitter& Save::target()
            {
               return m_emitter;
            }


            namespace writer
            {

               Implementation::Implementation( YAML::Emitter& output) : m_output( output)
               {
                  m_output << YAML::BeginMap;
               }


               std::size_t Implementation::container_start( const std::size_t size, const char* const name)
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

            } // writer

         } // yaml
      } // archive
   } // sf
} // casual

