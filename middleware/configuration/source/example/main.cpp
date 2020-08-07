//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "configuration/domain.h"
#include "configuration/example/domain.h"

#include "configuration/example/build/server.h"
#include "configuration/example/build/executable.h"
#include "configuration/example/resource/property.h"

#include "common/serialize/macro.h"
#include "common/serialize/create.h"


#include "common/argument.h"
#include "common/exception/guard.h"

namespace casual
{
   namespace configuration
   {
      namespace example
      {
         namespace create
         {

            void domain( const std::string& file)
            {
               write( example::domain(), file);
            }

            void default_domain( const std::string& file)
            {
               write( domain::Manager{}, file);
            }

            namespace build
            {
               void server( const std::string& file)
               {
                  example::build::server::write( example::build::server::example(), file);
               }

               void executable( const std::string& file)
               {
                  example::build::executable::write( example::build::executable::example(), file);
               }

               void default_server( const std::string& file)
               {
                  example::build::server::write( configuration::build::server::Server{}, file);
               }
            } // build

            namespace resource
            {
               void property( const std::string& file)
               {
                  example::resource::property::write( example::resource::property::example(), file);
               }

               void default_server( const std::string& file)
               {
                  example::resource::property::write( {}, file);
               }

            } // resource

         } // create

         void main( int argc, char **argv)
         {
            using namespace common::argument;

            Parse{
               R"(
Produces configuration examples from object models

the output format will be deduced from file extension

)",
               Option( &create::domain, { "-d", "--domain-file"}, "domain configuration example"),
               Option( &create::default_domain, { "-dd", "--default-domain-file"}, "default domain configuration example"),

               Option( &create::build::server, { "-b", "--build-server-file"}, "build server configuration example"),
               Option( &create::build::default_server, { "-db", "--default-build-server-file"}, "default build server configuration example"),

               Option( &create::build::executable, { "-e", "--build-executable-file"}, "build executable configuration example"),

               Option( &create::resource::property, { "-r", "--resource-property-file"}, "resource property configuration example"),
               Option( &create::resource::default_server, { "-dr", "--default-resource-property-file"}, "default resource property configuration example")

            }( argc, argv);
         }

      } // example

   } // configuration

} // casual

int main( int argc, char **argv)
{
   return casual::common::exception::main::guard( std::cerr, [=]()
   {
      casual::configuration::example::main( argc, argv);
   });
}
