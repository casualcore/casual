//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "configuration/model.h"
#include "configuration/model/load.h"

#include "common/unittest/file.h"

namespace casual
{
   namespace configuration::unittest
   {
      //! loads model from yaml contents.
      template< typename... C>
      auto load( C&&... contents)
      {
         auto get_path = []( auto& file){ return static_cast< std::filesystem::path>( file);};

         auto files = common::unittest::file::temporary::contents( ".yaml", std::forward< C>( contents)...);
         return casual::configuration::model::load( common::algorithm::transform( files, get_path));
      }
      
   } // configuration::unittest
} // casual