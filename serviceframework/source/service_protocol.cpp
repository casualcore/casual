//!
//! service_protocol.cpp
//!
//! Created on: May 5, 2013
//!     Author: Lazan
//!

#include "sf/service_protocol.h"

namespace casual
{
   namespace sf
   {
      namespace service
      {
         namespace protocol
         {


            Base::Base( TPSVCINFO* serviceInfo) : m_info( serviceInfo) {}

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


         }
      } // protocol
   } // sf
} // casual


