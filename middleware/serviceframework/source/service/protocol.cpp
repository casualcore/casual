//!
//! casual
//!

#include "sf/service/protocol.h"
#include "sf/log.h"

#include "common/execution.h"

#include "xatmi.h"

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


            bool Base::do_call()
            {
               return true;
            }

            reply::State Base::do_finalize()
            {
               return m_state;
            }

            void Base::do_handle_exception()
            {
               m_state.value = TPFAIL;

               common::error::handler();
            }

            Interface::Input& Base::do_input()
            {
               return m_input;
            }

            Interface::Output& Base::do_output()
            {
               return m_output;
            }

            Binary::Binary( TPSVCINFO* serviceInfo) : Base( serviceInfo),
                  m_readerBuffer( buffer::raw( serviceInfo)), m_reader( m_readerBuffer), m_writer( m_writerBuffer)
            {
               sf::Trace trace{ "protocol::Binary::Binary"};

               m_input.readers.push_back( &m_reader);
               m_output.writers.push_back( &m_writer);

            }

            const std::string& Binary::type()
            {
               return buffer::type::binary();
            }

            reply::State Binary::do_finalize()
            {
               sf::Trace trace{ "protocol::Binary::do_finalize"};

               auto raw = m_writerBuffer.release();
               m_state.data = raw.buffer;
               m_state.size = raw.size;

               sf::log::sf << "state: " << m_state << '\n';

               return m_state;
            }


            Yaml::Yaml( TPSVCINFO* serviceInfo)
               : Base( serviceInfo), m_reader( m_load( serviceInfo->data, serviceInfo->len)), m_writer( m_save())
            {
               sf::Trace trace{ "protocol::Yaml::doYaml"};

               m_input.readers.push_back( &m_reader);
               m_output.writers.push_back( &m_writer);

               //
               // We don't need the request-buffer any more
               //
               tpfree( serviceInfo->data);
               serviceInfo->len = 0;

            }

            const std::string& Yaml::type()
            {
               return buffer::type::yaml();
            }

            reply::State Yaml::do_finalize()
            {
               sf::Trace trace{ "protocol::Yaml::do_finalize"};

               //
               // TODO: Let yaml::Save know about X_Octet (or binary::Stream)
               //


               std::string yaml;
               m_save( yaml);

               buffer::Binary buffer{ buffer::type::yaml(), yaml.size()};
               buffer.str( yaml);

               auto raw = buffer.release();
               m_state.data = raw.buffer;
               m_state.size = raw.size;

               sf::log::sf << "state: " << m_state << '\n';

               return m_state;
            }

            Json::Json( TPSVCINFO* serviceInfo)
               : Base( serviceInfo), m_reader( m_load( serviceInfo->data, serviceInfo->len)), m_writer( m_save())
            {
               sf::Trace trace{ "protocol::Json::Json"};

               m_input.readers.push_back( &m_reader);
               m_output.writers.push_back( &m_writer);

               //
               // We don't need the request-buffer any more
               //
               tpfree( serviceInfo->data);
               serviceInfo->len = 0;

            }

            const std::string& Json::type()
            {
               return buffer::type::json();
            }

            reply::State Json::do_finalize()
            {
               sf::Trace trace{ "protocol::Json::do_finalize"};

               //
               // TODO: Let json::Save know about X_Octet (or binary::Stream)
               //

               std::string json;
               m_save( json);

               buffer::Binary buffer{ buffer::type::json(), json.size()};
               buffer.str( json);

               auto raw = buffer.release();
               m_state.data = raw.buffer;
               m_state.size = raw.size;

               sf::log::sf << "state: " << m_state << '\n';

               return m_state;
            }


            Xml::Xml( TPSVCINFO* service_info)
               : Base( service_info), m_reader( m_load( service_info->data, service_info->len)), m_writer( m_save())
            {
               sf::Trace trace{ "protocol::Xml::Xml"};

               m_input.readers.push_back( &m_reader);
               m_output.writers.push_back( &m_writer);

               //
               // We don't need the request-buffer any more
               //
               tpfree( service_info->data);
               service_info->len = 0;

            }


            const std::string& Xml::type()
            {
               return common::buffer::type::xml();
            }

            reply::State Xml::do_finalize()
            {
               sf::Trace trace{ "protocol::Xml::do_finalize"};

               //
               // TODO: Let xml::Save know about X_Octet (or binary::Stream)
               //

               std::string xml;
               m_save( xml);

               buffer::Binary buffer{ common::buffer::type::xml(), xml.size()};
               buffer.str( xml);

               auto raw = buffer.release();
               m_state.data = raw.buffer;
               m_state.size = raw.size;

               sf::log::sf << "state: " << m_state << '\n';

               return m_state;
            }


            Ini::Ini( TPSVCINFO* service_info)
            : Base( service_info), m_reader( m_load( service_info->data, service_info->len)), m_writer( m_save())
            {
               sf::Trace trace{ "protocol::Ini::Ini"};

               m_input.readers.push_back( &m_reader);
               m_output.writers.push_back( &m_writer);

               //
               // We don't need the request-buffer any more
               //
               tpfree( service_info->data);
               service_info->len = 0;
            }

            reply::State Ini::do_finalize()
            {
               std::string ini;
               m_save( ini);

               buffer::Binary buffer{ common::buffer::type::ini(), ini.size()};
               buffer.str( ini);

               auto raw = buffer.release();
               m_state.data = raw.buffer;
               m_state.size = raw.size;

               return m_state;
            }

            const std::string& Ini::type()
            {
               return common::buffer::type::ini();
            }



            Describe::Describe( TPSVCINFO* information, std::unique_ptr< Interface>&& protocol)
                  : Base( information), m_writer( m_model), m_protocol( std::move( protocol))
            {
               sf::Trace trace{ "protocol::Describe::Describe"};

               m_model.service = common::execution::service::name();

               m_input.readers.push_back( &m_prepare);
               m_input.writers.push_back( &m_writer.input);

               m_output.readers.push_back( &m_prepare);
               m_output.writers.push_back( &m_writer.output);

            }


            bool Describe::do_call()
            {
               return false;
            }

            reply::State Describe::do_finalize()
            {
               sf::Trace trace{ "protocol::Describe::do_finalize"};

               service::IO service_io{ std::move( m_protocol)};

               service_io << name::value::pair::make( "model", m_model);

               return service_io.finalize();
            }

         } // protocol
      } // service
   } // sf
} // casual


