//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "configuration/domain.h"

namespace casual
{
   namespace configuration
   {
      namespace example
      {
         configuration::domain::Manager domain();

         //! queue centric documentation
         configuration::queue::Manager queue();

         void write( const configuration::domain::Manager& domain, const std::string& file);

         common::file::scoped::Path temporary( const configuration::domain::Manager& domain, const std::string& extension);

      } // example

   } // configuration


} // casual


