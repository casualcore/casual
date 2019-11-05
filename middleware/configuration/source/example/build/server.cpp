//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "configuration/example/build/server.h"

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
            namespace server
            {

               model_type example()
               {
                  model_type result;

                  result.server_default.service.transaction.emplace( "join");
                  result.server_default.service.category.emplace( "some.category");


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

                  result.services = {
                        [](){
                           configuration::build::server::Service v;
                           v.name = "s1";
                           return v;
                        }(),
                        [](){
                           configuration::build::server::Service v;
                           v.name = "s2";
                           v.transaction.emplace( "auto");
                           return v;
                        }(),
                        [](){
                           configuration::build::server::Service v;
                           v.name = "s3";
                           v.function.emplace( "f3");
                           return v;
                        }(),
                        [](){
                           configuration::build::server::Service v;
                           v.name = "s4";
                           v.function.emplace( "f4");
                           v.category.emplace( "some.other.category");
                           return v;
                        }()
                  };

                  return result;
               }

               void write( const model_type& server, const std::string& name)
               {
                  common::file::Output file{ name};
                  auto archive = common::serialize::create::writer::from( file.extension(), file);
                  
                  archive << CASUAL_NAMED_VALUE( server);
               }

               common::file::scoped::Path temporary(const model_type& server, const std::string& extension)
               {
                  common::file::scoped::Path file{ common::file::name::unique( common::directory::temporary() + "/build_server_", extension)};

                   write( server, file);

                  return file;
               }

            } // server
         } // build
      } // example
   } // configuration
} // casual
