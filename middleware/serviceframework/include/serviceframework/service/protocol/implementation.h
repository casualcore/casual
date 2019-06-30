//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "serviceframework/service/protocol.h"
#include "serviceframework/service/model.h"

#include "serviceframework/service/protocol/describe.h"
#include "serviceframework/log.h"

#include "common/serialize/archive.h"
#include "common/serialize/log.h"

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
                  common::serialize::Reader m_reader;
                  common::serialize::Writer m_writer;

               };

               class Yaml : public Base
               {
               public:

                  Yaml( protocol::parameter_type&& parameter);

                  protocol::result_type finalize();
                  static const std::string& type();

               private:
                  common::serialize::Reader m_reader;
                  common::serialize::Writer m_writer;
               };

               class Json : public Base
               {
               public:

                  Json( protocol::parameter_type&& parameter);

                  protocol::result_type finalize();
                  static const std::string& type();

               private:
                  common::serialize::Reader m_reader;
                  common::serialize::Writer m_writer;
               };

               class Xml : public Base
               {
               public:

                  Xml( protocol::parameter_type&& parameter);

                  protocol::result_type finalize();
                  static const std::string& type();

               private:
                  common::serialize::Reader m_reader;
                  common::serialize::Writer m_writer;
               };

               class Ini : public Base
               {
               public:
                  Ini( protocol::parameter_type&& parameter);

                  protocol::result_type finalize();
                  static const std::string& type();

               private:
                  common::serialize::Reader m_reader;
                  common::serialize::Writer m_writer;
               };

               namespace parameter
               {
                  template< typename B>
                  class Log : public B
                  {
                  public:
                     using base_type = B;

                     Log( protocol::parameter_type&& parameter) 
                        : base_type( std::move( parameter)), m_writer( common::serialize::log::writer( log::parameter))
                     {
                        this->m_input.writers.push_back( &m_writer);
                        this->m_output.writers.push_back( &m_writer);
                     }

                     Log( Log&&) = default;

                  private:
                     common::serialize::Writer m_writer;

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

                  common::serialize::Reader m_prepare = service::protocol::describe::prepare();

                  struct writer_t
                  {
                     writer_t( Model& model) 
                        : input( service::protocol::describe::writer( model.arguments.input)), 
                          output( service::protocol::describe::writer( model.arguments.output)) {}

                     common::serialize::Writer input;
                     common::serialize::Writer output;
                  } m_writer;

                  service::Protocol m_protocol;
               };

            } // implementation
         } // protocol
      } // service
   } // serviceframework
} // casual



