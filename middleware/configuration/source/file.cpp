//!
//! casual
//!

#include "configuration/file.h"
#include "common/file.h"
#include "common/environment.h"

namespace casual
{
   namespace configuration
   {

      namespace directory
      {

         std::string domain()
         {
            return common::environment::directory::domain() + "/configuration";
         }

         std::string persistent()
         {
            return domain() + "/.persistent";
         }

      } // directory

      namespace file
      {


         std::string find( const std::string& basename)
         {
            return find( common::directory::name::base( basename), common::file::name::base( basename));
         }

         std::string find( const std::string& path, const std::string& basename)
         {
            return common::file::find( path, std::regex( basename + ".(yaml|yml|json|jsn|xml|ini)" ));
         }


         namespace persistent
         {
            std::string domain()
            {
               return directory::persistent() + "/domain.yaml";
            }
         } // persistent


      } // file
   } // config



} // casual
