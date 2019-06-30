//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "configuration/build/server.h"
#include "common/file.h"

namespace casual
{
   namespace configuration
   {
      namespace example
      {
         namespace build
         {
            namespace server
            {
               using model_type = configuration::build::server::Server;

               model_type example();

               void write( const model_type& model, const std::string& file);

               common::file::scoped::Path temporary(const model_type& model, const std::string& extension);

            } // server

         } // build

      } // example
   } // configuration
} // casual


