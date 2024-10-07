//! 
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/argument.h"
#include "common/pimpl.h"

namespace casual
{
   namespace administration::cli
   {
      std::vector< argument::Option> options();

      void parse( int argc, const char** argv);
      void parse( std::vector< std::string> options);

   } // administration
} // casual
