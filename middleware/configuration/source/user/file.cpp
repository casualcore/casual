//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "configuration/user/file.h"
#include "common/file.h"
#include "common/environment.h"

namespace casual
{
   namespace configuration
   {
      namespace user
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


         } // file
      } // user
   } // configuration
} // casual
