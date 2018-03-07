//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MOCKUP_FILE_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MOCKUP_FILE_H_

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

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MOCKUP_FILE_H_
