//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "configuration/user.h"
#include "configuration/example/model.h"

#include "configuration/example/build/model.h"

#include "common/serialize/create.h"
#include "common/serialize/macro.h"
#include "common/file.h"

#include "common/argument.h"
#include "common/exception/guard.h"

#include <fstream>
#include <filesystem>

namespace casual
{
   namespace configuration::documentation::samples
   {
      namespace local
      {
         namespace
         {
            auto file( const std::filesystem::path& path)
            {
               // make sure we create directories if not present
               common::directory::create( path.parent_path());
               return std::ofstream{ path};
            }

            template< typename C>
            void generate( std::filesystem::path file, std::string_view format, const C& configuration)
            {               
               auto writer = common::serialize::create::writer::from( format);
               writer << configuration;
               auto out = local::file( file.replace_extension( format));
               writer.consume( out);
            }

            template< typename C>
            void generate( std::filesystem::path file, const C& configuration)
            {
               for( auto format : { "yaml", "json", "xml", "ini"})
                  local::generate( file, format, configuration);
            }

            namespace domain
            {
               void general( const std::filesystem::path& root)
               {
                  generate( root / "sample/domain/general.format", example::user::part::domain::general());
               }

               void transaction( const std::filesystem::path& root)
               {        
                  generate( root / "sample/domain/transaction.format", example::user::part::domain::transaction());
               }


               void queue( const std::filesystem::path& root)
               {
                  generate( root / "sample/domain/queue.format", example::user::part::domain::queue());
               }

               void gateway( const std::filesystem::path& root)
               {
                  generate( root / "sample/domain/gateway.format", example::user::part::domain::gateway());
               }

            } // domain
         
            void system( const std::filesystem::path& root)
            {
               generate( root / "sample/system.format",  example::user::part::system());
            }

            namespace build
            {
               void server( const std::filesystem::path& root)
               {
                  generate( root / "sample/build/server.format",  example::build::model::server());
               }

               void executable( const std::filesystem::path& root)
               {
                  generate( root / "sample/build/executable.format",  example::build::model::executable());
               }
               
            } // build


            void main( int argc, char **argv)
            {
               std::string root;

               {
                  using namespace common::argument;

                  Parse{
                     R"(Produces configuration documentation)",
                     Option( std::tie( root), { "--root"}, "the root of where documentation will be generated"),
                  }( argc, argv);

                  if( root.empty())
                     return;
               }

               domain::general( root);
               domain::transaction( root);
               domain::queue( root);
               domain::gateway( root);

               system( root);
               build::server( root);
               build::executable( root);
            }
         } // <unnamed>
      } // local

   } // configuration::documentation::samples

} // casual

int main( int argc, char **argv)
{
   return casual::common::exception::main::cli::guard( [=]()
   {
      casual::configuration::documentation::samples::local::main( argc, argv);
   });
}
