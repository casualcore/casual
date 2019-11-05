//! 
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "configuration/example/build/executable.h"

#include "common/serialize/create.h"

#include <fstream>

namespace casual
{
   namespace configuration
   {
      namespace example
      {
         namespace build
         {
            namespace executable
            {

               configuration::build::Executable example()
               {
                  configuration::build::Executable result;

                  result.entrypoint = "start";
                  result.resources = {
                     []()
                     {
                        configuration::build::Resource v;
                        v.key = "rm-mockup";
                        v.name = "resource-1";
                        v.note = "the runtime configuration for this resource is correlated with the name 'resource-1' - no group is needed for resource configuration";
                        return v;
                     }()
                  };
                  return result;
               }

               void write( const configuration::build::Executable& executable, const std::string& name)
               {
                  common::file::Output file{ name};
                  auto archive = common::serialize::create::writer::from( file.extension(), file);
                  
                  archive << CASUAL_NAMED_VALUE( executable);
               }

            } // executable
         } // build
      } // example
   } // configuration
} // casual
