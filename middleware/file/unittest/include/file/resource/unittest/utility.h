//! 
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "file/manager/admin/model.h"

namespace casual
{
   namespace file::resource::unittest
   {
      manager::admin::model::State state();

      namespace blocking
      {
         namespace reserve
         {
            void send( std::filesystem::path path);
            std::filesystem::path receive();
         } // reserve
      } // blocking

   } // file::resource::unittest
} // casual
