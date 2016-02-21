//!
//! casual 
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
            common::file::scoped::Path temporary( const std::string& extension, const std::string& content)
            {
               common::file::scoped::Path path{ common::file::name::unique( "mockup-", extension)};
               std::ofstream file{ path};
               file << content;
               return path;
            }
         } // file

      } // mockup
   } // common


} // casual
