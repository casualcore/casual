//!
//! casual
//!

#include "config/file.h"

#include "common/file.h"
#include "common/environment.h"

namespace casual
{
   namespace config
   {

      namespace directory
      {

         std::string domain()
         {
            return common::environment::directory::domain() + "/configuration";
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

         std::string domain()
         {
            return find( directory::domain(), "domain");
         }


         std::string gateway()
         {
            return find( directory::domain(), "gateway");
         }




      } // file
   } // config



} // casual
