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
   namespace serviceframework::service::protocol::implementation
   {
      struct Base : common::traits::unrelocatable
      {
         Base( protocol::payload_type&& payload);
         Base();

         bool call() const;
         void exception();

         io::Input& input();
         io::Output& output();

      protected:

         protocol::payload_type reuse_payload();

         

         //protocol::parameter_type m_parameter;
         protocol::payload_type m_payload;

         io::Input m_input;
         io::Output m_output;
      };


      struct Binary : public Base
      {
         Binary( protocol::payload_type&& payload);
         static constexpr auto type() { return common::buffer::type::binary;}
         protocol::payload_type finalize();

      private:
         common::serialize::Reader m_reader;
         common::serialize::Writer m_writer;

      };

      struct Yaml : public Base
      {
         Yaml( protocol::payload_type&& payload);

         protocol::payload_type finalize();
         static constexpr auto type() { return common::buffer::type::yaml;}

      private:
         common::serialize::Reader m_reader;
         common::serialize::Writer m_writer;
      };

      struct Json : public Base
      {
         Json( protocol::payload_type&& payload);

         protocol::payload_type finalize();
         static constexpr auto type() { return common::buffer::type::json;}

      private:
         common::serialize::Reader m_reader;
         common::serialize::Writer m_writer;
      };

      struct Xml : public Base
      {
         Xml( protocol::payload_type&& payload);

         protocol::payload_type finalize();
         static constexpr auto type() { return common::buffer::type::xml;}

      private:
         common::serialize::Reader m_reader;
         common::serialize::Writer m_writer;
      };

      struct Ini : public Base
      {
         Ini( protocol::payload_type&& payload);

         protocol::payload_type finalize();
         static constexpr auto type() { return common::buffer::type::ini;}

      private:
         common::serialize::Reader m_reader;
         common::serialize::Writer m_writer;
      };

      namespace parameter
      {
         struct Log : common::traits::unrelocatable
         {
            Log( service::Protocol&& protocol);

            inline decltype( auto) type() const { return m_protocol.type();}

            bool call();
            protocol::payload_type finalize();
            inline void exception() { m_protocol.exception();}

            inline io::Input& input() { return m_protocol.input();}
            inline io::Output& output() { return m_protocol.output();}

         private:
            service::Protocol m_protocol;
            common::serialize::Writer m_writer;
            
         };
      } // parameter


      struct Describe : common::traits::unrelocatable
      {
         Describe( service::Protocol&& protocol);

         bool call() const;
         protocol::payload_type finalize();
         inline decltype( auto) type() const { return m_protocol.type();}

         inline void exception() { m_protocol.exception();}

         inline io::Input& input() { return m_input;}
         inline io::Output& output() { return m_output;}

      private:

         io::Input m_input;
         io::Output m_output;

         Model m_model;

         common::serialize::Reader m_prepare = service::protocol::describe::prepare();

         struct Writer
         {
            Writer( Model& model) 
               : input( service::protocol::describe::writer( model.arguments.input)), 
                  output( service::protocol::describe::writer( model.arguments.output)) {}

            common::serialize::Writer input;
            common::serialize::Writer output;
         } m_writer;

         service::Protocol m_protocol;
      };

   } // serviceframework::service::protocol::implementation
} // casual



