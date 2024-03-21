//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/domain.h"

#include "common/environment.h"
#include "common/log.h"
#include "common/string.h"

#include "common/code/raise.h"
#include "common/code/casual.h"
#include "common/code/system.h"

#include "common/serialize/json.h"

// std
#include <fstream>

namespace casual
{
   namespace common::domain
   {

      Identity::Identity() = default;

      Identity::Identity( const strong::domain::id& id, std::string name)
         : id{ id}, name{ std::move( name)}
      {}

      Identity::Identity( std::string name)
         : Identity{ strong::domain::id{ uuid::make()}, std::move( name)}
      {}

      namespace local
      {
         namespace
         {
            namespace variable
            {
               namespace name
               {
                  constexpr auto information = "CASUAL_DOMAIN_INFORMATION";
               } // name
               
            } // variable

            Identity identity = environment::variable::get< Identity>( variable::name::information).value_or( Identity{});

            namespace singleton
            {
               domain::singleton::Model read( const std::filesystem::path& path)
               {
                  Trace trace{ "common::domain::singleton::read"};

                  log::line( log::debug, "path: ", path);

                  std::ifstream file{ path};

                  if( ! file)
                     return {};

                  domain::singleton::Model model;
                  auto archive = serialize::json::relaxed::reader( file);
                  archive >> model;

                  common::domain::identity( model.identity);

                  environment::variable::set( environment::variable::name::ipc::domain::manager, model.process);
                  
                  log::line( log::debug, "domain singleton model: ", model);

                  return model; 
               }
            } // singleton
         }
      } // local

      const Identity& identity()
      {
         return local::identity;
      }

      void identity( Identity value)
      {
         local::identity = std::move( value);
         common::environment::variable::set( local::variable::name::information, local::identity);
      }

      namespace singleton
      {
         file::scoped::Path create()
         {
            Trace trace{ "common::domain::singleton::create"};

            const Model model{ process::handle(), domain::identity()};

            auto path = directory::create_parent_path( environment::domain::singleton::file());

            if( std::filesystem::exists( path))
            {
               auto content = local::singleton::read( path);

               log::line( log::category::verbose::error, "domain lock file: ", path, ", content: ", content);

               // There is potentially a running casual-domain already - abort
               code::raise::error( code::casual::domain_running, "can only be one casual-domain-manager running in a domain");
            }

            file::scoped::Path temp_file{ file::name::unique( path.string(), ".tmp")};
            {
               std::ofstream output( temp_file);
               if ( ! output)
                  code::raise::error( code::casual::invalid_file, "cannot create domain lock file: ", temp_file.string(), ", errno: ", code::system::last::error());

               auto archive = serialize::json::writer();
               archive << model;
               archive.consume( output);

               if ( ! output)
                  code::raise::error( code::casual::invalid_file, "cannot write to domain lock file: ", temp_file.string(), ", errno: ", code::system::last::error());
            }

            log::line( log::debug, "domain singleton model: ", model);

            common::file::rename( temp_file, path);
            temp_file.release();

            return { std::move( path)};
         }


         Model read()
         {
            return local::singleton::read( common::environment::domain::singleton::file());
         }

      } // singleton

   } // common::domain
} // casual
