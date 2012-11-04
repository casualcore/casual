//!
//! casual_archive_yaml_policy.h
//!
//! Created on: Oct 31, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_ARCHIVE_YAML_POLICY_H_
#define CASUAL_ARCHIVE_YAML_POLICY_H_

#include "casual_policy_base.h"
#include "casual_sf_basic_archive.h"

#include <yaml-cpp/yaml.h>

#include <istream>

namespace casual
{

   namespace sf
   {

      namespace policy
      {
         namespace yaml
         {
            class Reader : public Base
            {
            public:

               Reader( std::istream& input)
               {
                  YAML::Parser parser( input);

                  if( !parser.GetNextDocument( m_document))
                  {
                     // TODO: Handle error
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
                     // TODO: Handle with policy
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
                  if( m_nodeStack.back()->Type() == YAML::NodeType::Map)
                  {
                     const YAML::Node* valueNode = m_nodeStack.back()->FindValue( m_currentRole);

                     if( valueNode)
                     {
                        (*valueNode) >> value;
                     }
                     else
                     {
                        // TODO:
                     }

                  }
                  else
                  {
                     (*m_nodeStack.back()) >> value;
                     m_nodeStack.pop_back();
                  }

               }



               void read( const std::vector< char>& value)
               {
                  // do nada
               }


            private:

               YAML::Node m_document;
               std::deque< const YAML::Node*> m_nodeStack;
               const char* m_currentRole;
            };


            class Writer
            {

            public:
               Writer( YAML::Emitter& output) : m_output( output)
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
                     m_output << YAML::Value << value;
                  }
                  else
                  {
                     m_output << value;
                  }
               }



               void write( const std::vector< char>& value)
               {
                  // do nada
               }


            private:

               YAML::Emitter& m_output;
               std::string m_currentRole;
               std::stack< YAML::EMITTER_MANIP> m_emitterStack;


            };


         }

      } // policy

      namespace archive
      {

         typedef archive::basic_reader< policy::yaml::Reader> YamlReader;

      }

      namespace archive
      {

         typedef archive::basic_writer< policy::yaml::Writer> YamlWriter;

      }

   } // sf
} // casual



#endif /* CASUAL_ARCHIVE_YAML_POLICY_H_ */
