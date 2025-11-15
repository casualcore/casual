//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/serialize/service/protocol/implementation.h"


#include "common/serialize/binary.h"
#include "common/serialize/json.h"
#include "common/serialize/toml.h"
#include "common/serialize/yaml.h"
#include "common/serialize/xml.h"
#include "common/serialize/log.h"
#include "common/serialize/create.h"

#include "common/execution/context.h"
#include "common/exception/capture.h"
#include "common/environment.h"

#include "xatmi.h"

namespace casual
{
   namespace common::serialize::service::protocol::implementation
   {

      Base::Base( protocol::payload_type&& payload)
         : m_payload{ std::move( payload)}
      {}

      bool Base::call() const
      {
         return true;
      }

      void Base::exception()
      {
         common::exception::sink();
         //m_result.code.result = decltype( m_result.code.result)::fail;
      }

      io::Input& Base::input() { return m_input;}
      io::Output& Base::output() { return m_output;}

      protocol::payload_type Base::reuse_payload()
      {
         auto result = std::move( m_payload);
         result.data.clear();
         return result;
      }


      Binary::Binary( protocol::payload_type&& payload)
         : Base( std::move( payload)), 
            m_reader( common::serialize::binary::reader( m_payload.data)), 
            m_writer( common::serialize::binary::writer())
      {
         Trace trace{ "protocol::Binary::Binary"};

         m_input.readers.push_back( &m_reader);
         m_output.writers.push_back( &m_writer);

      }

      protocol::payload_type Binary::finalize()
      {
         // resuse the payload memory
         auto result = Base::reuse_payload();
         m_writer.consume( result.data);
         return result;
      }


      Json::Json( protocol::payload_type&& payload)
         : Base( std::move( payload)),
            m_reader{ common::serialize::json::relaxed::reader( m_payload.data)},
            m_writer{ common::serialize::json::writer()}
      {
         Trace trace{ "protocol::Json::Json"};

         m_input.readers.push_back( &m_reader);
         m_output.writers.push_back( &m_writer);
      }

      protocol::payload_type Json::finalize()
      {
         Trace trace{ "protocol::Json::finalize"};

         auto result = Base::reuse_payload();
         m_writer.consume( result.data);
         return result;
      }


      Toml::Toml( protocol::payload_type&& payload)
         : Base( std::move( payload)),
            m_reader{ common::serialize::toml::relaxed::reader( m_payload.data)},
            m_writer{ common::serialize::toml::writer()}
      {
         Trace trace{ "protocol::Toml::Toml"};

         m_input.readers.push_back( &m_reader);
         m_output.writers.push_back( &m_writer);
      }

      protocol::payload_type Toml::finalize()
      {
         Trace trace{ "protocol::Toml::finalize"};

         auto result = Base::reuse_payload();
         m_writer.consume( result.data);
         return result;
      }


      Yaml::Yaml( protocol::payload_type&& payload)
         : Base( std::move( payload)),
            m_reader{ common::serialize::yaml::relaxed::reader( m_payload.data)},
            m_writer{ common::serialize::yaml::writer()}
      {
         Trace trace{ "protocol::Yaml::Yaml"};

         m_input.readers.push_back( &m_reader);
         m_output.writers.push_back( &m_writer);
      }

      protocol::payload_type Yaml::finalize()
      {
         Trace trace{ "protocol::Yaml::finalize"};

         auto result = Base::reuse_payload();
         m_writer.consume( result.data);
         return result;
      }

      Xml::Xml( protocol::payload_type&& payload)
         : Base( std::move( payload)),
            m_reader{ common::serialize::xml::relaxed::reader( m_payload.data)},
            m_writer{ common::serialize::xml::writer()}
      {
         Trace trace{ "protocol::Xml::Xml"};

         m_input.readers.push_back( &m_reader);
         m_output.writers.push_back( &m_writer);
      }

      protocol::payload_type Xml::finalize()
      {
         Trace trace{ "protocol::Xml::finalize"};

         auto result = Base::reuse_payload();
         m_writer.consume( result.data);
         return result;
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

                  auto parameter_format = common::environment::variable::get( 
                     common::environment::variable::name::log::parameter::format).value_or( "line");

                  common::log::debug( "parameter format: ", parameter_format);

                  return common::serialize::create::writer::from( parameter_format);
               }

            } // <unnamed>
         } // local
         
         Log::Log( service::Protocol&& protocol) 
            : m_protocol{ std::move( protocol)}, m_writer{ local::writer()}
         {
            Trace trace{ "protocol::parameter::Log::Log"};
            common::log::debug( "protocol: ", m_protocol);

            m_protocol.input().writers.push_back( &m_writer);
            m_protocol.output().writers.push_back( &m_writer);
         }

         bool Log::call() 
         {
            Trace trace{ "protocol::implementation::parameter::Log::call"};

            m_writer.consume( common::log::category::parameter);
            static_cast< std::ostream&>( common::log::category::parameter) << "\n";

            return m_protocol.call();
         }

         protocol::payload_type Log::finalize() 
         { 
            Trace trace{ "protocol::implementation::parameter::Log::finalize"};

            m_writer.consume( common::log::category::parameter);
            static_cast< std::ostream&>( common::log::category::parameter) << "\n";

            return m_protocol.finalize();
         }

      } // parameter


      Describe::Describe( service::Protocol&& protocol)
            :  m_writer( m_model), m_protocol( std::move( protocol))
      {
         Trace trace{ "protocol::Describe::Describe"};
         common::log::debug( "protocol: ", m_protocol);

         m_model.service = common::execution::context::get().service;

         m_input.readers.push_back( &m_prepare);
         m_input.writers.push_back( &m_writer.input);

         m_output.readers.push_back( &m_prepare);
         m_output.writers.push_back( &m_writer.output);
      }


      bool Describe::call() const
      {
         return false;
      }

      protocol::payload_type Describe::finalize()
      {
         Trace trace{ "protocol::Describe::finalize"};

         m_protocol << common::serialize::named::value::make( m_model, "model");

         return m_protocol.finalize();
      }
   } // serviceframework::service::protocol::implementation
} // casual


