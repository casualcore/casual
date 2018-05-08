//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "serviceframework/service/protocol.h"
#include "serviceframework/service/model.h"

#include "serviceframework/archive/yaml.h"
#include "serviceframework/archive/binary.h"
#include "serviceframework/archive/json.h"
#include "serviceframework/archive/xml.h"
#include "serviceframework/archive/ini.h"
#include "serviceframework/archive/log.h"
#include "serviceframework/archive/service.h"
#include "serviceframework/log.h"

namespace casual
{
   namespace serviceframework
   {
      namespace service
      {
         namespace protocol
         {
            namespace implementation
            {
               class Base
               {

               public:
                  Base( protocol::parameter_type&& parameter);
                  Base( Base&&);

                  bool call() const;

                  protocol::result_type finalize();

                  void exception();

                  io::Input& input();
                  io::Output& output();

               protected:

                  protocol::parameter_type m_parameter;
                  protocol::result_type m_result;

                  io::Input m_input;
                  io::Output m_output;
               };


               class Binary : public Base
               {
               public:
                  Binary( protocol::parameter_type&& parameter);
                  Binary( Binary&&);

                  static const std::string& type();

               private:
                  archive::Reader m_reader;
                  archive::Writer m_writer;

               };

               class Yaml : public Base
               {
               public:

                  Yaml( protocol::parameter_type&& parameter);

                  protocol::result_type finalize();
                  static const std::string& type();

               private:
                  archive::Reader m_reader;
                  archive::Writer m_writer;
               };

               class Json : public Base
               {
               public:

                  Json( protocol::parameter_type&& parameter);

                  protocol::result_type finalize();
                  static const std::string& type();

               private:
                  archive::Reader m_reader;
                  archive::Writer m_writer;
               };

               class Xml : public Base
               {
               public:

                  Xml( protocol::parameter_type&& parameter);

                  protocol::result_type finalize();
                  static const std::string& type();

               private:
                  archive::Reader m_reader;
                  archive::Writer m_writer;
               };

               class Ini : public Base
               {
               public:
                  Ini( protocol::parameter_type&& parameter);

                  protocol::result_type finalize();
                  static const std::string& type();

               private:
                  archive::Reader m_reader;
                  archive::Writer m_writer;
               };



               namespace parameter
               {
                  template< typename B>
                  class Log : public B
                  {
                  public:
                     using base_type = B;

                     Log( protocol::parameter_type&& parameter) : base_type( std::move( parameter)), m_writer( archive::log::writer( log::parameter))
                     {
                        this->m_input.writers.push_back( &m_writer);
                        this->m_output.writers.push_back( &m_writer);
                     }

                     Log( Log&&) = default;

                  private:
                     archive::Writer m_writer;

                  };
               } // parameter

               class Describe
               {
               public:

                  Describe( service::Protocol&& protocol);

                  Describe( Describe&& other);
                  Describe& operator = ( Describe&& other);


                  bool call() const;
                  protocol::result_type finalize();
                  const std::string& type() const;

                  void exception() { m_protocol.exception();}

                  inline io::Input& input() { return m_input;}
                  inline io::Output& output() { return m_output;}

               private:

                  void setup();

                  io::Input m_input;
                  io::Output m_output;

                  Model m_model;

                  archive::Reader m_prepare = archive::service::describe::prepare();

                  struct writer_t
                  {
                     writer_t( Model& model) 
                        : input( archive::service::describe::writer( model.arguments.input)), 
                          output( archive::service::describe::writer( model.arguments.output)) {}

                     archive::Writer input;
                     archive::Writer output;
                  } m_writer;

                  service::Protocol m_protocol;
               };

            } // implementation
         } // protocol
      } // service
   } // serviceframework
} // casual



