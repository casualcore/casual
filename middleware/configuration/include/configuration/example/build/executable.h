//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "configuration/build/executable.h"
#include "common/file.h"

namespace casual
{
   namespace configuration
   {
      namespace example
      {
         namespace build
         {
            namespace executable
            {
               configuration::build::Executable example();

               void write( const configuration::build::Executable& model, const std::string& file);

            } // executable

         } // build
      } // example
   } // configuration
} // casual


