//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/log/stream.h"
#include "casual/argument.h"

#include <string>
#include <optional>
#include <iostream>

namespace casual
{
   namespace cli::state
   {
      template< typename S>
      void serialize( const S& state, const std::optional< std::string>& format)
      {
         auto serialize_to_stdout = []( auto&& archive, auto& state)
         {
            archive << state;
            archive.consume( std::cout);
         };

         if( format == "line")
            common::log::write( std::cout, state);
         else
            serialize_to_stdout( common::serialize::create::writer::from( format.value_or( "")), state);

         std::cout << '\n';
      }      

      template< typename C>
      auto option( C state_callback)
      {
         auto invoke = [ callback = std::move( state_callback)]( const std::optional< std::string>& format)
         {
            state::serialize( callback(), format);
         };

         auto formats = []( bool, auto){ return std::vector< std::string>{ "json", "yaml", "xml", "ini", "line"};};

         return argument::Option{
            std::move( invoke),
            std::move( formats),
            { "--state"},
            "prints state in the provided format to stdout"
         };
      }
      
   } // cli::state
} // casual