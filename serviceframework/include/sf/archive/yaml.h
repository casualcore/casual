//!
//! casual_archive_yaml_policy.h
//!
//! Created on: Oct 31, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_ARCHIVE_YAML_POLICY_H_
#define CASUAL_ARCHIVE_YAML_POLICY_H_

#include "sf/archive/basic.h"
#include "sf/reader_policy.h"

#include <yaml-cpp/yaml.h>
#include <yaml-cpp/binary.h>

#include <fstream>
#include <stack>

namespace casual
{
   namespace sf
   {
      namespace archive
      {
         namespace yaml
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
               class Implementation
               {
               public:

                  enum class State
                  {
                     unknown,
                     container,
                     serializable,
                     missing,
                  };

                  typedef P policy_type;

                  Implementation( std::istream& input)
                  {
                     YAML::Parser parser( input);

                     if( ! parser.GetNextDocument( m_document))
                     {
                        m_policy.initalization( "empty yaml document");
                     }

                     m_nodeStack.push( &m_document);
                     m_state.push( State::unknown);
                  }

                  inline void handle_start( const char* name)
                  {
                     m_currentRole = name;
                  }

                  inline void handle_end( const char* name) { /* no op */}

                  inline std::size_t handle_container_start( std::size_t size)
                  {

                     const YAML::Node* containerNode = nullptr;

                     switch( m_state.top())
                     {
                        case State::container:
                        {
                           containerNode = m_nodeStack.top();
                           break;
                        }
                        case State::unknown:
                        case State::serializable:
                        {
                           containerNode = m_nodeStack.top()->FindValue( m_currentRole);
                           break;
                        }
                        case State::missing:
                        {
                           break;
                        }
                     }

                     if( containerNode)
                     {

                        size = containerNode->size();

                        for( std::size_t index = size; index > 0; --index)
                        {
                           m_nodeStack.push( &(*containerNode)[ index - 1] );
                        }
                        m_state.push( State::container);
                     }
                     else
                     {
                        size = m_policy.container( m_currentRole);
                        m_state.push( State::missing);
                     }

                     return size;
                  }

                  inline void handle_container_end()
                  {
                     m_state.pop();

                  }

                  inline void handle_serialtype_start()
                  {
                     switch( m_state.top())
                     {
                        case State::container:
                        {
                           //
                           // We are in a container and the nodes are already pushed
                           //
                           m_state.push( State::serializable);
                           break;
                        }
                        case State::unknown:
                        case State::serializable:
                        {
                           const YAML::Node* serialNode = m_nodeStack.top()->FindValue( m_currentRole);
                           if( serialNode != nullptr)
                           {
                              m_nodeStack.push( serialNode);
                              m_state.push( State::serializable);
                           }
                           else
                           {
                              m_policy.serialtype( m_currentRole);
                              m_state.push( State::missing);
                           }
                           break;
                        }
                        case State::missing:
                        {
                           m_state.push( State::missing);
                           break;
                        }
                     }
                  }

                  inline void handle_serialtype_end()
                  {
                     //
                     // Always pop, if not missing
                     //
                     if( m_state.top() != State::missing)
                     {
                        m_nodeStack.pop();
                     }
                     m_state.pop();
                  }

                  template< typename T>
                  void read( T& value)
                  {
                     switch( m_state.top())
                     {
                        case State::container:
                        {
                           readValue(*m_nodeStack.top(), value);
                           m_nodeStack.pop();
                           break;
                        }
                        case State::unknown:
                        case State::serializable:
                        {
                           const YAML::Node* valueNode = m_nodeStack.top()->FindValue( m_currentRole);
                           if( valueNode)
                           {
                              readValue( *valueNode, value);
                           }
                           else
                           {
                              m_policy.value( m_currentRole, value);
                           }
                           break;
                        }
                        default:
                        {
                           // no op
                           break;
                        }
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

                  void readValue( const YAML::Node& node, platform::binary_type& value)
                  {
                     YAML::Binary binary;
                     node >> binary;
                     copyBinary( binary, value);

                  }




                  YAML::Node m_document;
                  std::stack< const YAML::Node*> m_nodeStack;
                  std::stack< State> m_state;
                  const char* m_currentRole = nullptr;

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



               class Implementation
               {

               public:

                  typedef YAML::Emitter buffer_type;

                  Implementation( YAML::Emitter& output) : m_output( output)
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

                  void writeValue( const platform::binary_type& value)
                  {
                     // TODO: can we cast an be conformant?
                     const unsigned char* data = reinterpret_cast< const unsigned char*>( value.data());
                     YAML::Binary binary( data, value.size());
                     m_output << binary;
                  }

                  YAML::Emitter& m_output;
                  std::string m_currentRole;
                  std::stack< YAML::EMITTER_MANIP> m_emitterStack;
               };

            } // writer


            typedef basic_reader< reader::Implementation< policy::reader::Strict> > Reader;

            namespace relaxed
            {
               typedef basic_reader< reader::Implementation< policy::reader::Relaxed> > Reader;
            }


            typedef basic_writer< writer::Implementation> Writer;

         } // yaml
      } // archive
   } // sf
} // casual



#endif /* CASUAL_ARCHIVE_YAML_POLICY_H_ */
