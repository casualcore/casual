//!
//! service_protocol.cpp
//!
//! Created on: May 5, 2013
//!     Author: Lazan
//!

#include "sf/service_protocol.h"

#include "common/trace.h"


namespace casual
{
   namespace sf
   {
      namespace service
      {
         namespace protocol
         {


            Base::Base( TPSVCINFO* serviceInfo)
               : m_info( serviceInfo)
            {
               m_state.value = TPSUCCESS;
            }

            Base::Base( Base&&) = default;


            bool Base::doCall()
            {
               return true;
            }

            reply::State Base::doFinalize()
            {
               return m_state;
            }

            void Base::doHandleException()
            {

            }

            Interface::Input& Base::doInput()
            {
               return m_input;
            }

            Interface::Output& Base::doOutput()
            {
               return m_output;
            }


            Binary::Binary( TPSVCINFO* serviceInfo) : Base( serviceInfo),
                  m_readerBuffer( buffer::raw( serviceInfo)), m_reader( m_readerBuffer), m_writer( m_writerBuffer)
            {
               common::Trace trace{ "Binary::Binary"};

               m_input.readers.push_back( &m_reader);
               m_output.writers.push_back( &m_writer);

            }

            reply::State Binary::doFinalize()
            {
               common::Trace trace{ "Binary::doFinalize"};

               auto raw = m_writerBuffer.release();
               m_state.data = raw.buffer;
               m_state.size = raw.size;

               return m_state;
            }


            Yaml::Yaml( TPSVCINFO* serviceInfo) : Base( serviceInfo),
               m_inputstream( serviceInfo->data), m_reader( m_inputstream), m_writer( m_outputstream)
            {
               //
               // We don't need the request-buffer no more
               //
               tpfree( serviceInfo->data);
               serviceInfo->len = 0;


               m_input.readers.push_back( &m_reader);
               m_output.writers.push_back( &m_writer);

            }

            reply::State Yaml::doFinalize()
            {
               buffer::X_Octet buffer{ "YAML", m_outputstream.size() };

               buffer.str( m_outputstream.c_str());

               buffer::Raw raw = buffer.release();
               m_state.data = raw.buffer;
               m_state.size = raw.size;

               return m_state;
            }

            Json::Json( TPSVCINFO* serviceInfo) : Base( serviceInfo),
                  m_reader( serviceInfo->data), m_writer( m_root)
            {
               common::Trace trace{ "Json::Json"};
               m_input.readers.push_back( &m_reader);
               m_output.writers.push_back( &m_writer);
            }

            reply::State Json::doFinalize()
            {
               common::Trace trace{ "Json::doFinalize"};
               const std::string json{ json_object_to_json_string( m_root) };

               buffer::X_Octet buffer{ "JSON", json.size() };
               buffer.str( json);

               buffer::Raw raw = buffer.release();
               m_state.data = raw.buffer;
               m_state.size = raw.size;

               //
               // Free buffer
               //
               tpfree( m_info->data);

               return m_state;
            }


         }
      } // protocol
   } // sf
} // casual


