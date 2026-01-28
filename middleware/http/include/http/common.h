//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/log.h"
#include "common/code/xatmi.h"
#include "common/buffer/type.h"

namespace casual
{
   namespace http
   {
      using Trace = common::Trace;

      namespace header
      {
         namespace name
         {
            namespace result
            {
               inline constexpr auto code = "casual-result-code";
   
               namespace user
               {
                  inline constexpr auto code = "casual-result-user-code";
               } // user
            } // result

            namespace execution
            {
               inline constexpr auto id = "casual-execution-id";

               namespace trace 
               {
                  inline constexpr std::string_view parent = "traceparent";
               } // trace

            } // execution

         } // name


         namespace value::result
         {
            common::code::xatmi code( std::string_view value);
            std::string_view code( common::code::xatmi code);

            namespace user
            {
               long code( std::string_view value);
               std::string code( long code);
            } // user

         } // value::result
      } // header

      namespace protocol
      {
         constexpr std::string_view x_octet = "application/casual-x-octet";
         constexpr std::string_view binary = "application/casual-binary";
         constexpr std::string_view json = "application/json";
         constexpr std::string_view yaml = "application/yaml";
         constexpr std::string_view toml = "application/toml";
         constexpr std::string_view xml = "application/xml";
         constexpr std::string_view field = "application/casual-field";
         constexpr std::string_view order = "application/casual-order";
         constexpr std::string_view string = "application/casual-string";
         constexpr std::string_view null = "application/casual-null"; 

         namespace convert
         {
            namespace to
            {
               std::string_view content( std::string_view buffer);
               std::string_view buffer( std::string_view content);
            } // to
         } // convert
      } //protocol

   } // http
} // casual



