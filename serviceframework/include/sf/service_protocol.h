//!
//! service_implementation.h
//!
//! Created on: Jan 4, 2013
//!     Author: Lazan
//!

#ifndef SERVICE_IMPLEMENTATION_H_
#define SERVICE_IMPLEMENTATION_H_

#include "sf/service.h"


#include "sf/archive_yaml.h"

namespace casual
{
   namespace sf
   {
      namespace service
      {
         namespace protocol
         {
            class Base : public Interface
            {

            public:
               Base( TPSVCINFO* serviceInfo);

            private:

               bool doCall() override;

               reply::State doFinalize() override;

               void doHandleException() override;

               Interface::Input& doInput() override;

               Interface::Output& doOutput() override;


            protected:

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

               Yaml( TPSVCINFO* serviceInfo);

               reply::State doFinalize() override;

            private:

               std::istringstream m_inputstream;
               archive::yaml::Reader m_reader;
               YAML::Emitter m_outputstream;
               archive::yaml::Writer m_writer;
            };


         } // protocol
      } // service
   } // sf
} // casual


#endif /* SERVICE_IMPLEMENTATION_H_ */
