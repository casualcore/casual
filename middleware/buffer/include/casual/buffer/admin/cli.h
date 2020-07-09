//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/argument.h"
#include "common/pimpl.h"
#include "common/buffer/type.h"

#include <string>
#include <vector>

namespace casual
{
   namespace buffer
   {
      namespace admin
      {
         namespace cli
         {

            //! only exposed to support the deprecated 'buffer-tooling-executables'
            namespace detail
            {
               namespace format
               {
                  auto completion()
                  {
                     return []( auto values, bool) -> std::vector< std::string>
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
                              common::buffer::type::x_octet(), 
                              common::buffer::type::binary(),
                              common::buffer::type::yaml(),
                              common::buffer::type::xml(),
                              common::buffer::type::json(),
                              common::buffer::type::ini()};
                        };
                     };                  
                  } // types
               } // buffer

               namespace field
               {
                  void from_human( const common::optional< std::string>& format);
                  void to_human( const common::optional< std::string>& format);


               } // field

               void compose( const common::optional< std::string>& type);
               void duplicate( platform::size::type count);
               void extract();

            } // detail
         } // cli

         struct CLI 
         {
            CLI();
            ~CLI();

            common::argument::Group options() &;

         private:
            struct Implementation;
            common::move::basic_pimpl< Implementation> m_implementation;
         };
      } // admin
   } // buffer
} // casual