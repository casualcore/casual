//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/file.h"

namespace casual
{
   namespace common
   {
      namespace mockup
      {

         namespace file
         {
            namespace temporary
            {
               common::file::scoped::Path content( const std::string& extension, const std::string& content);

               common::file::scoped::Path name( const std::string& extension);
            } // temporary
         } // file

      } // mockup
   } // common


} // casual


