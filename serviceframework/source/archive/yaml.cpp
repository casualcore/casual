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
               YAML::Parser parser( stream);

               if( ! parser.GetNextDocument( m_document))
               {
                  throw exception::archive::invalid::Document{ "invalid yaml document"};
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
               Implementation::Implementation( const YAML::Node& node) : m_nodeStack{ &node} {}

               std::tuple< std::size_t, bool> Implementation::container_start( std::size_t size, const char* const name)
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

               void Implementation::container_end( const char* const name)
               {

               }

               bool Implementation::serialtype_start( const char* const name)
               {
                  if( name)
                  {
                     m_nodeStack.push_back( m_nodeStack.back()->FindValue( name));
                  }

                  return m_nodeStack.back() != nullptr;
               }

               void Implementation::serialtype_end( const char* const name)
               {
                  m_nodeStack.pop_back();
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

