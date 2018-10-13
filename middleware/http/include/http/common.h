//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/log/trace.h"
#include "common/code/xatmi.h"

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
         namespace name
         {
            namespace result
            {
               constexpr auto code = "casual-result-code";

               namespace user
               {
                  constexpr auto code = "casual-result-user-code";
               } // user

            } // result
         } // name

         namespace value
         {
            namespace result
            {
               common::code::xatmi code( const std::string& value);
               const char* code( common::code::xatmi code);

               namespace user
               {
                  long code( const std::string& value);
                  std::string code( long code);
               } // user
            } // result            
         } // value
      } // header

      namespace protocol
      {

         const std::string& x_octet();
         const std::string& binary();
         const std::string& json();
         const std::string& xml();
         const std::string& field();


         namespace convert
         {
            namespace from
            {
               std::string buffer( const std::string& buffer);
            } // from

            namespace to
            {
               std::string buffer( const std::string& content);
            } // to
         } // convert

      } //protocol
   } // http
} // casual



