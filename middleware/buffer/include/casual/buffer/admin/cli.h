//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/argument.h"
#include "common/pimpl.h"
#include "common/buffer/type.h"

#include <string>
#include <vector>

namespace casual
{
   namespace buffer::admin
   {
      namespace cli
      {

         //! only exposed to support the deprecated 'buffer-tooling-executables'
         namespace detail
         {
            namespace format
            {
               inline auto completion()
               {
                  return []( bool, auto values) -> std::vector< std::string>
                  {
                     return { "json", "yaml", "xml", "ini"};
                  };
               }
                              
            } // format
            namespace buffer
            {
               namespace types
               {
                  auto completion()
                  {
                     return []( auto values, auto help) -> std::vector< std::string> 
                     {
                        return {
                           std::string( common::buffer::type::x_octet), 
                           std::string( common::buffer::type::binary),
                           std::string( common::buffer::type::yaml),
                           std::string( common::buffer::type::xml),
                           std::string( common::buffer::type::json),
                           std::string( common::buffer::type::ini)};
                     };
                  };                  
               } // types
            } // buffer

            namespace field
            {
               void from_human( std::optional< std::string> format);
               void to_human( std::optional< std::string> format);


            } // field

            void compose( const std::optional< std::string>& type);
            void duplicate( platform::size::type count);
            void extract();

         } // detail

         argument::Option options();
      } // cli


   } // buffer::admin
} // casual