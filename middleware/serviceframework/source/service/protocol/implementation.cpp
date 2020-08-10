//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "serviceframework/service/protocol/implementation.h"
#include "serviceframework/log.h"

#include "common/serialize/binary.h"
#include "common/serialize/json.h"
#include "common/serialize/xml.h"
#include "common/serialize/yaml.h"
#include "common/serialize/ini.h"
#include "common/serialize/log.h"
#include "common/serialize/create.h"

#include "common/execution.h"
#include "common/exception/handle.h"
#include "common/environment.h"

#include "xatmi.h"

namespace casual
{
   using namespace common;
   
   namespace serviceframework
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

               bool Base::call() const
               {
                  return true;
               }

               void Base::exception()
               {
                  m_result.transaction = decltype( m_result.transaction)::rollback;
                  exception::sink::log();
               }

               io::Input& Base::input() { return m_input;}
               io::Output& Base::output() { return m_output;}

               
               protocol::result_type Base::finalize()
               {
                  Trace trace{ "protocol::implementation::Base::finalize"};
                  return std::move( m_result);
               }



               Binary::Binary( protocol::parameter_type&& parameter)
                  : Base( std::move( parameter)),
                     m_reader( serialize::binary::reader( m_parameter.payload.memory)), 
                     m_writer( serialize::binary::writer())
               {
                  Trace trace{ "protocol::Binary::Binary"};

                  m_input.readers.push_back( &m_reader);
                  m_output.writers.push_back( &m_writer);

               }

               const std::string& Binary::type()
               {
                  return buffer::type::binary();
               }

               protocol::result_type Binary::finalize()
               {
                  auto result = Base::finalize();
                  m_writer.consume( result.payload.memory);
                  return result;
               }


               Yaml::Yaml( protocol::parameter_type&& parameter)
                  : Base( std::move( parameter)),
                    m_reader{ serialize::yaml::relaxed::reader( m_parameter.payload.memory)},
                    m_writer{ serialize::yaml::writer()}
               {
                  Trace trace{ "protocol::Yaml::Yaml"};

                  m_input.readers.push_back( &m_reader);
                  m_output.writers.push_back( &m_writer);

                  // We don't need the request-buffer any more, we can use the memory though...
                  m_result.payload = std::move( m_parameter.payload);
                  m_result.payload.memory.clear();
               }

               const std::string& Yaml::type()
               {
                  return buffer::type::yaml();
               }

               protocol::result_type Yaml::finalize()
               {
                  Trace trace{ "protocol::Yaml::finalize"};

                  auto result = Base::finalize();
                  m_writer.consume( result.payload.memory);
                  return result;
               }


               Json::Json( protocol::parameter_type&& parameter)
                  : Base( std::move( parameter)),
                    m_reader{ common::serialize::json::relaxed::reader( m_parameter.payload.memory)},
                    m_writer{ common::serialize::json::writer()}
               {
                  Trace trace{ "protocol::Json::Json"};

                  m_input.readers.push_back( &m_reader);
                  m_output.writers.push_back( &m_writer);

                  // We don't need the request-buffer any more, we can use the memory though...
                  m_result.payload = std::move( m_parameter.payload);
                  m_result.payload.memory.clear();
               }

               const std::string& Json::type()
               {
                  return buffer::type::json();
               }

               protocol::result_type Json::finalize()
               {
                  Trace trace{ "protocol::Json::finalize"};

                  auto result = Base::finalize();
                  m_writer.consume( result.payload.memory);
                  return result;
               }



               Xml::Xml( protocol::parameter_type&& parameter)
                  : Base( std::move( parameter)),
                    m_reader{ serialize::xml::relaxed::reader( m_parameter.payload.memory)},
                    m_writer{ serialize::xml::writer()}
               {
                  Trace trace{ "protocol::Xml::Xml"};

                  m_input.readers.push_back( &m_reader);
                  m_output.writers.push_back( &m_writer);

                  // We don't need the request-buffer any more, we can use the memory though...
                  m_result.payload = std::move( m_parameter.payload);
                  m_result.payload.memory.clear();

               }


               const std::string& Xml::type()
               {
                  return buffer::type::xml();
               }

               protocol::result_type Xml::finalize()
               {
                  Trace trace{ "protocol::Xml::finalize"};

                  auto result = Base::finalize();
                  m_writer.consume( result.payload.memory);
                  return result;
               }


               Ini::Ini( protocol::parameter_type&& parameter)
               : Base( std::move( parameter)),
                 m_reader( serialize::ini::relaxed::reader( m_parameter.payload.memory)),
                 m_writer( serialize::ini::writer())
               {
                  Trace trace{ "protocol::Ini::Ini"};

                  m_input.readers.push_back( &m_reader);
                  m_output.writers.push_back( &m_writer);

                  // We don't need the request-buffer any more, we can use the memory though...
                  m_result.payload = std::move( m_parameter.payload);
                  m_result.payload.memory.clear();
               }

               protocol::result_type Ini::finalize()
               {
                  Trace trace{ "protocol::Ini::finalize"};

                  auto result = Base::finalize();
                  m_writer.consume( result.payload.memory);
                  return result;
               }

               const std::string& Ini::type()
               {
                  return buffer::type::ini();
               }


               namespace parameter
               {
                  namespace local
                  {
                     namespace
                     {
                        auto writer() 
                        { 
                           Trace trace{ "protocol::parameter::local::writer"};

                           auto parameter_format = environment::variable::get( 
                              environment::variable::name::log::parameter::format,
                              "line");

                           common::log::line( verbose::log, "parameter format: ", parameter_format);

                           return serialize::create::writer::from( parameter_format);
                        }

                     } // <unnamed>
                  } // local
                  
                  Log::Log( service::Protocol&& protocol) 
                     : m_protocol{ std::move( protocol)}, m_writer{ local::writer()}
                  {
                     Trace trace{ "protocol::parameter::Log::Log"};
                     common::log::line( verbose::log, "protocol: ", m_protocol);

                     m_protocol.input().writers.push_back( &m_writer);
                     m_protocol.output().writers.push_back( &m_writer);
                  }

                  bool Log::call() 
                  {
                     Trace trace{ "protocol::implementation::parameter::Log::call"};

                     m_writer.consume( log::parameter);
                     static_cast< std::ostream&>( log::parameter) << "\n";

                     return m_protocol.call();
                  }

                  protocol::result_type Log::finalize() 
                  { 
                     Trace trace{ "protocol::implementation::parameter::Log::finalize"};

                     m_writer.consume( log::parameter);
                     static_cast< std::ostream&>( log::parameter) << "\n";

                     return m_protocol.finalize();
                  }

               } // parameter


               Describe::Describe( service::Protocol&& protocol)
                     :  m_writer( m_model), m_protocol( std::move( protocol))
               {
                  Trace trace{ "protocol::Describe::Describe"};
                  common::log::line( verbose::log, "protocol: ", m_protocol);

                  m_model.service = execution::service::name();

                  m_input.readers.push_back( &m_prepare);
                  m_input.writers.push_back( &m_writer.input);

                  m_output.readers.push_back( &m_prepare);
                  m_output.writers.push_back( &m_writer.output);
               }


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
                  Trace trace{ "protocol::Describe::finalize"};

                  m_protocol << serialize::named::value::make( m_model, "model");

                  return m_protocol.finalize();
               }

            } // implementation

         } // protocol
      } // service
   } // serviceframework
} // casual


