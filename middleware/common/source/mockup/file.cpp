//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/mockup/file.h"


#include <fstream>

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
               common::file::scoped::Path name( const std::string& extension)
               {
                  return { common::file::name::unique( common::directory::temporary() + "/mockup-", extension)};
               }

               common::file::scoped::Path content( const std::string& extension, const std::string& content)
               {
                  auto path = temporary::name( extension);
                  std::ofstream file{ path};
                  file << content;
                  return path;
               }
            } // temporary

         } // file

      } // mockup
   } // common


} // casual
