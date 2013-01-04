//!
//! casual_archive_yaml_policy.h
//!
//! Created on: Oct 31, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_ARCHIVE_YAML_POLICY_H_
#define CASUAL_ARCHIVE_YAML_POLICY_H_

#include "sf/policy_base.h"
#include "sf/basic_archive.h"
#include "sf/reader_policy.h"
#include "common/types.h"

#include <yaml-cpp/yaml.h>
#include <yaml-cpp/binary.h>

#include <fstream>

namespace casual
{

   namespace sf
   {

      namespace policy
      {
         namespace reader
         {
            struct Buffer
            {
               //typedef S stream_type;

               Buffer( const std::string& file) : m_stream( file) {}

               std::istream& archiveBuffer()
               {
                  return m_stream;
               }

            private:
               std::ifstream m_stream;
            };


            template< typename P>
            class Yaml : public Base
            {
            public:

               typedef P policy_type;

               Yaml( std::istream& input)
               {
                  YAML::Parser parser( input);

                  if( !parser.GetNextDocument( m_document))
                  {
                     m_policy.initalization();
                  }

                  m_nodeStack.push_back( &m_document);
               }

               inline void handle_start( const char* name)
               {
                  m_currentRole = name;
               }

               inline void handle_end( const char* name) { /* no op */}

               inline std::size_t handle_container_start( std::size_t size)
               {
                  const YAML::Node* containerNode = m_nodeStack.back()->FindValue( m_currentRole);

                  m_nodeStack.push_back( containerNode);

                  if( containerNode)
                  {
                     size = containerNode->size();

                     for( std::size_t index = size; index > 0; --index)
                     {
                        m_nodeStack.push_back( &(*containerNode)[ index - 1] );
                     }

                  }
                  else
                  {
                     size = m_policy.container( m_currentRole);
                  }



                  return size;
               }

               inline void handle_container_end()
               {
                  m_nodeStack.pop_back();

               }

               inline void handle_serialtype_start()
               {
                  if( m_nodeStack.back()->Type() == YAML::NodeType::Map)
                  {
                     const YAML::Node* serialNode = m_nodeStack.back()->FindValue( m_currentRole);
                     if( serialNode != nullptr)
                     {
                        m_nodeStack.push_back( serialNode);
                     }
                     else
                     {
                        m_policy.serialtype( m_currentRole);
                     }

                  }
                  else
                  {
                     m_nodeStack.push_back(  m_nodeStack.back());
                  }


               }

               inline void handle_serialtype_end()
               {
                  m_nodeStack.pop_back();
               }

               template< typename T>
               void read( T& value)
               {
                  typedef T value_type;
                  if( m_nodeStack.back()->Type() == YAML::NodeType::Map)
                  {
                     const YAML::Node* valueNode = m_nodeStack.back()->FindValue( m_currentRole);

                     if( valueNode)
                     {
                        readValue( *valueNode, value);
                     }
                     else
                     {
                        m_policy.value( m_currentRole, value);
                     }

                  }
                  else
                  {
                     readValue(*m_nodeStack.back(), value);
                     m_nodeStack.pop_back();
                  }
               }

            private:

               template< typename C>
               void copyBinary( YAML::Binary& binary, C& container)
               {
                  const unsigned char* data = binary.data();
                  container.assign( data, data + binary.size());
               }


               void copyBinary( YAML::Binary& binary, std::vector< unsigned char>& container)
               {
                  binary.swap( container);
               }


               template< typename T>
               void readValue( const YAML::Node& node, T& value)
               {
                  node >> value;
               }

               void readValue( const YAML::Node& node, common::binary_type& value)
               {
                  YAML::Binary binary;
                  node >> binary;
                  copyBinary( binary, value);

               }

               YAML::Node m_document;
               std::deque< const YAML::Node*> m_nodeStack;
               const char* m_currentRole;

               policy_type m_policy;
            };

         } // reader

         namespace writer
         {

            struct Buffer
            {
               //typedef S stream_type;

               Buffer( const std::string& file) : m_stream( file) {}

               std::istream& archiveBuffer()
               {
                  return m_stream;
               }

            private:
               std::ifstream m_stream;
            };


            class Yaml
            {

            public:
               Yaml( YAML::Emitter& output) : m_output( output)
               {
                  m_output << YAML::BeginMap;
                  m_emitterStack.push( YAML::BeginMap);
               }


               inline void handle_start( const char* name)
               {
                  m_currentRole = name;
               }

               inline void handle_end( const char* name) { /* no op */}

               inline std::size_t handle_container_start( std::size_t size)
               {
                  if( m_emitterStack.top() == YAML::BeginMap)
                  {
                     m_output << YAML::Key << m_currentRole;

                     m_output << YAML::Value;
                  }
                  m_output << YAML::BeginSeq;
                  m_emitterStack.push( YAML::BeginSeq);

                  return size;
               }

               inline void handle_container_end()
               {
                  m_output << YAML::EndSeq;
                  m_output << YAML::Newline;
                  m_emitterStack.pop();

               }

               inline void handle_serialtype_start()
               {
                  if( m_emitterStack.top() == YAML::BeginMap)
                  {
                     m_output << YAML::Key << m_currentRole;
                     m_output << YAML::Value;
                  }
                  m_output << YAML::BeginMap;
                  m_emitterStack.push( YAML::BeginMap);

               }

               inline void handle_serialtype_end()
               {
                  m_output << YAML::EndMap;
                  m_output << YAML::Newline;
                  m_emitterStack.pop();
               }

               template< typename T>
               void write( const T& value)
               {
                  if( m_emitterStack.top() == YAML::BeginMap)
                  {
                     m_output << YAML::Key << m_currentRole;
                     m_output << YAML::Value;
                  }
                  writeValue( value);
               }





            private:

               template< typename T>
               void writeValue( const T& value)
               {
                  m_output << value;
               }

               void writeValue( const common::binary_type& value)
               {
                  // TODO: can we cast an be conformant?
                  const unsigned char* data = value.empty() ? nullptr : reinterpret_cast< const unsigned char*>( &value[ 0]);
                  YAML::Binary binary( data, value.size());
                  m_output << binary;
               }




               YAML::Emitter& m_output;
               std::string m_currentRole;
               std::stack< YAML::EMITTER_MANIP> m_emitterStack;


            };


         } // writer

      } // policy

      namespace archive
      {
         namespace reader
         {


            typedef archive::basic_reader< policy::reader::Yaml< policy::reader::Relaxed> > YamlRelaxed;
            typedef archive::basic_reader< policy::reader::Yaml< policy::reader::Strict> > YamlStrict;

            /*
            namespace holder
            {
               typedef sf::archive::Holder< YamlRelaxed, policy::reader::Buffer<> > YamlRelaxed;
            }
            */

         }

      }

      namespace archive
      {
         namespace writer
         {
            typedef archive::basic_writer< policy::writer::Yaml> Yaml;
         }



      }

   } // sf
} // casual



#endif /* CASUAL_ARCHIVE_YAML_POLICY_H_ */
