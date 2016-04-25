//!
//! service_protocol.cpp
//!
//! Created on: May 5, 2013
//!     Author: Lazan
//!

#include "sf/service/protocol.h"

#include "common/internal/trace.h"


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
               const common::trace::internal::Scope trace{ "Binary::Binary"};

               m_input.readers.push_back( &m_reader);
               m_output.writers.push_back( &m_writer);

            }

            std::vector< buffer::Type> Binary::types()
            {
               return { buffer::type::binary()};
            }

            reply::State Binary::doFinalize()
            {
               const common::trace::internal::Scope trace{ "Binary::doFinalize"};

               auto raw = m_writerBuffer.release();
               m_state.data = raw.buffer;
               m_state.size = raw.size;

               return m_state;
            }


            Yaml::Yaml( TPSVCINFO* serviceInfo)
               : Base( serviceInfo), m_reader( m_load( serviceInfo->data, serviceInfo->len)), m_writer( m_save())
            {
               const common::trace::internal::Scope trace{ "Yaml::doYaml"};

               m_input.readers.push_back( &m_reader);
               m_output.writers.push_back( &m_writer);

               //
               // We don't need the request-buffer any more
               //
               tpfree( serviceInfo->data);
               serviceInfo->len = 0;

            }

            std::vector< buffer::Type> Yaml::types()
            {
               return { buffer::type::yaml()};
            }

            reply::State Yaml::doFinalize()
            {
               const common::trace::internal::Scope trace{ "Yaml::doFinalize"};

               //
               // TODO: Let yaml::Save know about X_Octet (or binary::Stream)
               //


               std::string yaml;
               m_save.serialize( yaml);

               buffer::Binary buffer{ buffer::type::yaml(), yaml.size()};
               buffer.str( yaml);

               auto raw = buffer.release();
               m_state.data = raw.buffer;
               m_state.size = raw.size;

               return m_state;
            }

            Json::Json( TPSVCINFO* serviceInfo)
               : Base( serviceInfo), m_reader( m_load( serviceInfo->data, serviceInfo->len)), m_writer( m_save())
            {
               const common::trace::internal::Scope trace{ "Json::Json"};

               m_input.readers.push_back( &m_reader);
               m_output.writers.push_back( &m_writer);

               //
               // We don't need the request-buffer any more
               //
               tpfree( serviceInfo->data);
               serviceInfo->len = 0;

            }

            std::vector< buffer::Type> Json::types()
            {
               return { buffer::type::json()};
            }

            reply::State Json::doFinalize()
            {
               const common::trace::internal::Scope trace{ "Json::doFinalize"};

               //
               // TODO: Let json::Save know about X_Octet (or binary::Stream)
               //

               std::string json;
               m_save.serialize( json);

               buffer::Binary buffer{ buffer::type::json(), json.size()};
               buffer.str( json);

               auto raw = buffer.release();
               m_state.data = raw.buffer;
               m_state.size = raw.size;

               return m_state;
            }


            Xml::Xml( TPSVCINFO* serviceInfo)
               : Base( serviceInfo), m_reader( m_load( serviceInfo->data, serviceInfo->len)), m_writer( m_save())
            {
               const common::trace::internal::Scope trace{ "Xml::Xml"};

               m_input.readers.push_back( &m_reader);
               m_output.writers.push_back( &m_writer);

               //
               // We don't need the request-buffer any more
               //
               tpfree( serviceInfo->data);
               serviceInfo->len = 0;

            }


            std::vector< buffer::Type> Xml::types()
            {
               return { common::buffer::type::xml()};
            }

            reply::State Xml::doFinalize()
            {
               const common::trace::internal::Scope trace{ "Xml::doFinalize"};

               //
               // TODO: Let xml::Save know about X_Octet (or binary::Stream)
               //

               std::string xml;
               m_save.serialize( xml);

               buffer::Binary buffer{ common::buffer::type::xml(), xml.size()};
               buffer.str( xml);

               auto raw = buffer.release();
               m_state.data = raw.buffer;
               m_state.size = raw.size;

               return m_state;
            }


            Describe::Describe( TPSVCINFO* information) : Base( information), m_writer( m_model), m_protocoll( protocoll( information))
            {
               const common::trace::internal::Scope trace{ "Describe::Describe"};

               m_input.readers.push_back( &m_prepare);
               m_input.writers.push_back( &m_writer.input);

               m_output.readers.push_back( &m_prepare);
               m_output.writers.push_back( &m_writer.output);

            }


            std::vector< buffer::Type> Describe::types()
            {
               return buffer::type::api::types();
            }

            namespace local
            {
               namespace
               {
                  std::map< buffer::Type, buffer::Type> api_mapping( std::vector< buffer::Type> types)
                  {
                     std::map< buffer::Type, buffer::Type> result;

                     for( auto& api_type : types)
                     {
                        buffer::Type type{ api_type.subname.c_str(), nullptr};
                        result[ std::move( api_type)] = std::move( type);
                     }

                     return result;
                  }
               } // <unnamed>
            } // local

            std::unique_ptr< Interface> Describe::protocoll( TPSVCINFO* information)
            {
               const static auto mapping = local::api_mapping( types());

               return service::Factory::instance().create( information, mapping.at( buffer::type::get( information->data)));
            }

            bool Describe::doCall()
            {
               return false;
            }

            reply::State Describe::doFinalize()
            {
               const common::trace::internal::Scope trace{ "Describe::doFinalize"};

               service::IO service_io{ std::move( m_protocoll)};

               service_io << makeNameValuePair( "model", m_model);

               return service_io.finalize();
            }

         }
      } // protocol
   } // sf
} // casual


