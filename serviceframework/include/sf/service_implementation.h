//!
//! service_implementation.h
//!
//! Created on: Jan 4, 2013
//!     Author: Lazan
//!

#ifndef SERVICE_IMPLEMENTATION_H_
#define SERVICE_IMPLEMENTATION_H_

#include "sf/service.h"


#include "sf/archive_yaml_implementation.h"

namespace casual
{
   namespace sf
   {
      namespace service
      {
         namespace implementation
         {
            class Base : public Interface
            {

            public:
               Base( TPSVCINFO* serviceInfo) : m_info( serviceInfo) {}

            private:

               bool doCall() override
               {
                  return true;
               }

               reply::State doFinalize() override
               {
                  return m_state;
               }

               void doHandleException() override
               {

               }

               Interface::Input& doInput() override
               {
                  return m_input;
               }

               Interface::Output& doOutput() override
               {
                  return m_output;
               }


            protected:

               /*
               template< typename IO>
               void finalize( IO& io)
               {
                  for( auto archive : io.readers)
                  {
                     archive->finalize();
                  }
                  for( auto archive : io.writers)
                  {
                     archive->finalize();
                  }
               }
               */

               TPSVCINFO* m_info;
               reply::State m_state;

               Interface::Input m_input;
               Interface::Output m_output;
            };


            class Binary : public Base
            {


            private:

            };

            class Yaml : public Base
            {
            public:

               Yaml( TPSVCINFO* serviceInfo) : Base( serviceInfo),
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

               reply::State doFinalize() override
               {
                  buffer::X_Octet buffer{ "YAML", m_outputstream.size() };

                  buffer.str( m_outputstream.c_str());

                  buffer::Raw raw = buffer.release();
                  m_state.data = raw.buffer;
                  m_state.size = raw.size;

                  return m_state;
               }

            private:

               std::istringstream m_inputstream;
               archive::yaml::reader::Strict m_reader;
               YAML::Emitter m_outputstream;
               archive::yaml::writer::Strict m_writer;


            };


         } // implementation

      } // service
   } // sf
} // casual


#endif /* SERVICE_IMPLEMENTATION_H_ */
