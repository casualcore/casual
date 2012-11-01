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

               Reader( std::istream& input) : m_parser( input) {}

               inline void handle_start( const char* name)
               {
                  m_currentRole = name;
               }

               inline void handle_end( const char* name) { /* no op */}

               inline void handle_container_start()
               {
               }

               inline void handle_container_end()
               {


               }

               inline void handle_serialtype_start()
               {


               }

               inline void handle_serialtype_end()
               {

               }

               template< typename T>
               void write( const T& value)
               {

               }



               void write( const std::vector< char>& value)
               {
                  // do nada
               }


            private:

               YAML::Parser m_parser;
               std::string m_currentRole;
            };


            class Writer : public Base
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

               inline void handle_container_start()
               {
                  if( m_emitterStack.top() == YAML::BeginMap)
                  {
                     m_output << YAML::Key << m_currentRole;

                     m_output << YAML::Value;
                  }
                  m_output << YAML::BeginSeq;
                  m_emitterStack.push( YAML::BeginSeq);
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

         typedef archive::basic_writer< policy::yaml::Writer> YamlWriter;

      }

   } // sf
} // casual



#endif /* CASUAL_ARCHIVE_YAML_POLICY_H_ */
