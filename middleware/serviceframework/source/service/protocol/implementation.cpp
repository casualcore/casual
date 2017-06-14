//!
//! casual
//!

#include "sf/service/protocol/implementation.h"
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
            namespace implementation
            {

               Base::Base( protocol::parameter_type&& parameter)
                  : m_parameter( std::move( parameter))
               {
                  m_result.payload.type = m_parameter.payload.type;
               }

               Base::Base( Base&&) = default;


               bool Base::call() const
               {
                  return true;
               }

               protocol::result_type Base::finalize()
               {
                  sf::Trace trace{ "protocol::Base::finalize"};

                  sf::log::sf << "result: " << m_result << '\n';

                  return std::move( m_result);
               }

               void Base::exception()
               {
                  m_result.transaction = common::service::invoke::Result::Transaction::rollback;
                  common::error::handler();
               }

               io::Input& Base::input() { return m_input;}
               io::Output& Base::output() { return m_output;}



               Binary::Binary( Binary&&) = default;

               Binary::Binary( protocol::parameter_type&& parameter)
                  : Base( std::move( parameter)),
                     m_reader( m_parameter.payload.memory), m_writer( m_result.payload.memory)
               {
                  sf::Trace trace{ "protocol::Binary::Binary"};

                  m_input.readers.push_back( &m_reader);
                  m_output.writers.push_back( &m_writer);

               }

               const std::string& Binary::type()
               {
                  return common::buffer::type::binary();
               }



               //Yaml::Yaml( Yaml&&) = default;

               Yaml::Yaml( protocol::parameter_type&& parameter)
                  : Base( std::move( parameter)),
                    m_reader( m_load( m_parameter.payload.memory)),
                    m_writer( m_save())
               {
                  sf::Trace trace{ "protocol::Yaml::Yaml"};

                  m_input.readers.push_back( &m_reader);
                  m_output.writers.push_back( &m_writer);

                  //
                  // We don't need the request-buffer any more, we can use the memory though...
                  //
                  m_result.payload = std::move( m_parameter.payload);
                  m_result.payload.memory.clear();
               }

               const std::string& Yaml::type()
               {
                  return common::buffer::type::yaml();
               }

               protocol::result_type Yaml::finalize()
               {
                  sf::Trace trace{ "protocol::Yaml::finalize"};

                  m_save( m_result.payload.memory);

                  return Base::finalize();
               }


               Json::Json( protocol::parameter_type&& parameter)
                  : Base( std::move( parameter)),
                    m_reader( m_load( m_parameter.payload.memory)),
                    m_writer( m_save())
               {
                  sf::Trace trace{ "protocol::Json::Json"};

                  m_input.readers.push_back( &m_reader);
                  m_output.writers.push_back( &m_writer);

                  //
                  // We don't need the request-buffer any more, we can use the memory though...
                  //
                  m_result.payload = std::move( m_parameter.payload);
                  m_result.payload.memory.clear();

               }

               const std::string& Json::type()
               {
                  return common::buffer::type::json();
               }

               protocol::result_type Json::finalize()
               {
                  sf::Trace trace{ "protocol::Json::finalize"};

                  m_save( m_result.payload.memory);
                  return Base::finalize();
               }



               Xml::Xml( protocol::parameter_type&& parameter)
                  : Base( std::move( parameter)),
                    m_reader( m_load( m_parameter.payload.memory)),
                    m_writer( m_save())
               {
                  sf::Trace trace{ "protocol::Xml::Xml"};

                  m_input.readers.push_back( &m_reader);
                  m_output.writers.push_back( &m_writer);

                  //
                  // We don't need the request-buffer any more, we can use the memory though...
                  //
                  m_result.payload = std::move( m_parameter.payload);
                  m_result.payload.memory.clear();

               }


               const std::string& Xml::type()
               {
                  return common::buffer::type::xml();
               }

               protocol::result_type Xml::finalize()
               {
                  sf::Trace trace{ "protocol::Xml::finalize"};

                  m_save( m_result.payload.memory);
                  return Base::finalize();
               }


               Ini::Ini( protocol::parameter_type&& parameter)
               : Base( std::move( parameter)),
                 m_reader( m_load( m_parameter.payload.memory)),
                 m_writer( m_save())
               {
                  sf::Trace trace{ "protocol::Ini::Ini"};

                  m_input.readers.push_back( &m_reader);
                  m_output.writers.push_back( &m_writer);

                  //
                  // We don't need the request-buffer any more, we can use the memory though...
                  //
                  m_result.payload = std::move( m_parameter.payload);
                  m_result.payload.memory.clear();
               }

               protocol::result_type Ini::finalize()
               {
                  sf::Trace trace{ "protocol::Ini::finalize"};

                  m_save( m_result.payload.memory);
                  return Base::finalize();
               }

               const std::string& Ini::type()
               {
                  return common::buffer::type::ini();
               }



               Describe::Describe( service::Protocol&& protocol)
                     :  m_writer( m_model), m_protocol( std::move( protocol))
               {
                  sf::Trace trace{ "protocol::Describe::Describe"};

                  setup();
               }

               Describe::Describe( Describe&& other)
                  : m_model( std::move( other.m_model)), m_writer( m_model), m_protocol( std::move( other.m_protocol))
               {
                  sf::Trace trace{ "protocol::Describe move ctor"};

                  setup();
               }

               void Describe::setup()
               {
                  m_model.service = common::execution::service::name();

                  m_input.readers.push_back( &m_prepare);
                  m_input.writers.push_back( &m_writer.input);

                  m_output.readers.push_back( &m_prepare);
                  m_output.writers.push_back( &m_writer.output);
               }

               //Describe& Describe::operator = ( Describe&& other);


               bool Describe::call() const
               {
                  return false;
               }

               const std::string& Describe::type() const
               {
                  return m_protocol.type();
               }

               protocol::result_type Describe::finalize()
               {
                  sf::Trace trace{ "protocol::Describe::finalize"};

                  m_protocol << name::value::pair::make( "model", m_model);

                  return m_protocol.finalize();
               }

            } // implementation

         } // protocol
      } // service
   } // sf
} // casual


