//!
//! casual
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


            bool Base::do_call()
            {
               return true;
            }

            reply::State Base::do_finalize()
            {
               return m_state;
            }

            void Base::do_andle_exception()
            {

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

            factory::buffer::Type Binary::type()
            {
               return buffer::type::binary();
            }

            reply::State Binary::do_finalize()
            {
               sf::Trace trace{ "protocol::Binary::do_finalize"};

               auto raw = m_writerBuffer.release();
               m_state.data = raw.buffer;
               m_state.size = raw.size;

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

            factory::buffer::Type Yaml::type()
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

            factory::buffer::Type Json::type()
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

               return m_state;
            }


            Xml::Xml( TPSVCINFO* serviceInfo)
               : Base( serviceInfo), m_reader( m_load( serviceInfo->data, serviceInfo->len)), m_writer( m_save())
            {
               sf::Trace trace{ "protocol::Xml::Xml"};

               m_input.readers.push_back( &m_reader);
               m_output.writers.push_back( &m_writer);

               //
               // We don't need the request-buffer any more
               //
               tpfree( serviceInfo->data);
               serviceInfo->len = 0;

            }


            factory::buffer::Type Xml::type()
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

               return m_state;
            }


            Describe::Describe( TPSVCINFO* information) : Base( information), m_writer( m_model), m_protocoll( protocoll( information))
            {
               sf::Trace trace{ "protocol::Describe::Describe"};

               m_input.readers.push_back( &m_prepare);
               m_input.writers.push_back( &m_writer.input);

               m_output.readers.push_back( &m_prepare);
               m_output.writers.push_back( &m_writer.output);

            }

            namespace local
            {
               namespace
               {
                  namespace describe
                  {
                     namespace type
                     {
                        bool equal( const common::buffer::Type& l, const common::buffer::Type& r)
                        {
                           return l.name == r.name;
                        }

                     } // type

                  } // describe

               } // <unnamed>
            } // local

            factory::buffer::Type Describe::type()
            {
               //
               // custom equal compare to delay the dispatch of the actual describe
               // protocol
               //
               return { buffer::type::api(), &local::describe::type::equal};
            }


            std::unique_ptr< Interface> Describe::protocoll( TPSVCINFO* information)
            {
               buffer::Type protocol_type{ buffer::type::get( information->data).subname, ""};

               return service::Factory::instance().create( information, protocol_type);
            }

            bool Describe::do_call()
            {
               return false;
            }

            reply::State Describe::do_finalize()
            {
               sf::Trace trace{ "protocol::Describe::do_finalize"};

               service::IO service_io{ std::move( m_protocoll)};

               service_io << makeNameValuePair( "model", m_model);

               return service_io.finalize();
            }

         }
      } // protocol
   } // sf
} // casual


