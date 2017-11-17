//!
//! casual 
//!

#include "configuration/domain.h"
#include "configuration/example/domain.h"

#include "configuration/example/build/server.h"
#include "configuration/example/resource/property.h"

#include "sf/namevaluepair.h"
#include "sf/archive/maker.h"


#include "common/arguments.h"
#include "common/exception/handle.h"

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

         int main( int argc, char **argv)
         {
            try
            {
               using namespace common::argument;

               common::Arguments argument{
                  R"(
Produces configuration examples from object models

the output format will be deduced from file extension

)",
                  {
                     directive( { "-d", "--domain-file"}, "domain configuration example", &create::domain),
                     directive( { "-dd", "--default-domain-file"}, "default domain configuration example", &create::default_domain),

                     directive( { "-b", "--build-server-file"}, "build server configuration example", &create::build::server),
                     directive( { "-db", "--default-build-server-file"}, "default build server configuration example", &create::build::default_server),

                     directive( { "-r", "--resource-property-file"}, "resource property configuration example", &create::resource::property),
                     directive( { "-dr", "--default-resource-property-file"}, "default resource property configuration example", &create::resource::default_server)

                  }
               };

               argument.parse( argc, argv);

               return 0;
            }
            catch( const std::exception& exception)
            {
               std::cerr << "excption: " << exception.what() << '\n';
               return 20;
            }
            catch( ...)
            {
               return common::exception::handle();
            }
         }

      } // example

   } // configuration

} // casual

int main( int argc, char **argv)
{
   return casual::configuration::example::main( argc, argv);
}
