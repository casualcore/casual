//!
//! casual_archive_yaml_policy.h
//!
//! Created on: Oct 31, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_ARCHIVE_YAML_POLICY_H_
#define CASUAL_ARCHIVE_YAML_POLICY_H_

#include "sf/archive/basic.h"
#include "sf/exception.h"
//#include "sf/reader_policy.h"

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

               class Implementation
               {
               public:

                  Implementation( std::istream& input)
                  {
                     YAML::Parser parser( input);

                     if( ! parser.GetNextDocument( m_document))
                     {
                        throw exception::archive::invalid::Document{ "invalid yaml document"};
                     }

                     m_nodeStack.push_back( &m_document);
                  }

                  inline std::tuple< std::size_t, bool> container_start( std::size_t size, const char* name)
                  {
                     const YAML::Node* node =  m_nodeStack.back();

                     if( node)
                     {
                        if( name)
                        {
                           node = node->FindValue( name);
                        }
                        else
                        {
                           //
                           // Indicates that we're in a container
                           //
                           m_nodeStack.pop_back();
                        }
                     }

                     if( ! node)
                     {
                        return std::make_tuple( 0, false);
                     }


                     //
                     // We stack'em i reverse order
                     //

                     size = node->size();

                     for( auto index = size; index > 0; --index)
                     {
                        m_nodeStack.push_back( &(*node)[ index - 1] );
                     }

                     return std::make_tuple( size, true);
                  }

                  inline void container_end( const char* name)
                  {

                  }

                  inline bool serialtype_start( const char* name)
                  {
                     if( name)
                     {
                        m_nodeStack.push_back( m_nodeStack.back()->FindValue( name));
                     }

                     return m_nodeStack.back() != nullptr;
                  }

                  inline void serialtype_end( const char* name)
                  {
                     m_nodeStack.pop_back();
                  }

                  template< typename T>
                  bool read( T& value, const char* name)
                  {
                     const YAML::Node* node = m_nodeStack.back();

                     if( node)
                     {
                        if( name)
                        {
                           node = node->FindValue( name);
                        }
                        else
                        {
                           //
                           // "unnamed", indicate we're in a container
                           // we pop
                           //
                           m_nodeStack.pop_back();
                        }
                     }

                     if( ! node)
                     {
                        return false;
                     }

                     readValue( *node, value);

                     return true;
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
                  std::vector< const YAML::Node*> m_nodeStack;
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
                  }


                  inline std::size_t container_start( std::size_t size, const char* name)
                  {
                     if( name)
                     {
                        m_output << YAML::Key << name;
                        m_output << YAML::Value;
                     }
                     m_output << YAML::BeginSeq;

                     return size;
                  }

                  inline void container_end( const char* name)
                  {
                     m_output << YAML::EndSeq;
                     m_output << YAML::Newline;
                  }

                  inline void serialtype_start( const char* name)
                  {
                     if( name)
                     {
                        m_output << YAML::Key << name;
                        m_output << YAML::Value;
                     }
                     m_output << YAML::BeginMap;
                  }

                  inline void serialtype_end( const char* name)
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
               };

            } // writer


            template< typename P>
            using basic_reader = archive::basic_reader< reader::Implementation, P>;

            using Reader = basic_reader< policy::Strict>;

            namespace relaxed
            {
               using Reader = basic_reader< policy::Relaxed>;
            }


            typedef basic_writer< writer::Implementation> Writer;

         } // yaml
      } // archive
   } // sf
} // casual



#endif /* CASUAL_ARCHIVE_YAML_POLICY_H_ */
