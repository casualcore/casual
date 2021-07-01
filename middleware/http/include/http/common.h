//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/log/trace.h"
#include "common/code/xatmi.h"
#include "common/buffer/type.h"

namespace casual
{
   namespace http
   {
      extern common::log::Stream log;

      namespace verbose
      {
         extern common::log::Stream log;
      } // verbose

      namespace trace
      {
         extern common::log::Stream log;
      } // trace

      struct Trace : common::log::Trace
      {
         template< typename T>
         Trace( T&& value) : common::log::Trace( std::forward< T>( value), trace::log) {}
      };

      namespace header
      {
         namespace name::result
         {
            inline constexpr auto code = "casual-result-code";
 
            namespace user
            {
               inline constexpr auto code = "casual-result-user-code";
            } // user
         } // name::result

         namespace value::result
         {
            common::code::xatmi code( std::string_view value);
            std::string_view code( common::code::xatmi code);

            namespace user
            {
               long code( const std::string& value);
               std::string code( long code);
            } // user

         } // value::result
      } // header

      namespace protocol
      {
         constexpr std::string_view x_octet = "application/casual-x-octet";
         constexpr std::string_view binary = "application/casual-binary";
         constexpr std::string_view json = "application/json";
         constexpr std::string_view xml = "application/xml";
         constexpr std::string_view field = "application/casual-field";
         constexpr std::string_view string = "application/casual-string";
         constexpr std::string_view null = "application/casual-null";

         namespace convert
         {
            namespace from
            {
               std::string buffer( std::string_view buffer);
            } // from

            namespace to
            {
               std::string buffer( std::string_view content);
            } // to
         } // convert
      } //protocol

      namespace buffer::transcode
      {
         namespace from
         {
            //! might base64 decode, based on buffer.type
            void wire( common::buffer::Payload& buffer);
         } // from

         namespace to
         {
            //! might base64 encode, based on buffer.type
            void wire( common::buffer::Payload& buffer);
         } // from 

      } // buffer::transcode

   } // http
} // casual



