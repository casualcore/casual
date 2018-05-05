//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "configuration/example/resource/property.h"

#include "serviceframework/archive/maker.h"

#include <fstream>

namespace casual
{
   namespace configuration
   {
      namespace example
      {
         namespace resource
         {
            namespace property
            {
               resources_type example()
               {
                  return {
                     { []( configuration::resource::Property& v){
                        v.key = "db2";
                        v.xa_struct_name = "db2xa_switch_static_std";
                        v.server = "rm-proxy-db2-static";
                        v.libraries = { "db2" };
                        v.paths.library = { "${DB2DIR}/lib64"};
                     }},
                     { []( configuration::resource::Property& v){
                        v.key = "rm-mockup";
                        v.xa_struct_name = "casual_mockup_xa_switch_static";
                        v.server = "rm-proxy-casual-mockup";
                        v.libraries = { "casual-mockup-rm" };
                        v.paths.library = { "${CASUAL_HOME}/lib"};
                     }}
                  };
               }

               void write( const resources_type& resources, const std::string& name)
               {
                  auto archive = serviceframework::archive::writer::from::file( name);
                  archive << CASUAL_MAKE_NVP( resources);
               }

               common::file::scoped::Path temporary(const resources_type& resources, const std::string& extension)
               {
                  common::file::scoped::Path file{ common::file::name::unique( common::directory::temporary() + "/domain_", extension)};

                  write( resources, file);

                  return file;
               }

            } // property

         } // resource

      } // example
   } // configuration
} // casual
