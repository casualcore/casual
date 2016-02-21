//!
//! casual 
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
            common::file::scoped::Path temporary( const std::string& extension, const std::string& content);
         } // file

      } // mockup
   } // common


} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MOCKUP_FILE_H_
