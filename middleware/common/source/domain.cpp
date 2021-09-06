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
      {
      }

      Identity::Identity( std::string name)
         : Identity{ strong::domain::id{ uuid::make()}, std::move( name)}
      {
      }


      bool operator == ( const Identity& lhs, const Identity& rhs)
      {
         return lhs.id == rhs.id;
      }

      bool operator < ( const Identity& lhs, const Identity& rhs)
      {
         return lhs.id < rhs.id;
      }


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

               void set( const Identity& identity)
               {
                  auto archive = serialize::json::writer();
                  archive << CASUAL_NAMED_VALUE( identity);
                  environment::variable::set( variable::name::information, archive.consume< std::string>());
               }

               Identity get()
               {
                  if( ! environment::variable::exists( variable::name::information))
                     return {};
                  
                  Identity identity;
                  auto archive = serialize::json::relaxed::reader( environment::variable::get( variable::name::information));
                  archive >> CASUAL_NAMED_VALUE( identity);
                  return identity;
               }
               
            } // variable

            Identity identity = variable::get();

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

                  
                  environment::variable::process::set( environment::variable::name::ipc::domain::manager, model.process);
                  common::domain::identity( model.identity);

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

         local::variable::set( local::identity);
      }

      namespace singleton
      {
         file::scoped::Path create()
         {
            Trace trace{ "common::domain::singleton::create"};

            const Model model{ process::handle(), domain::identity()};

            auto& path = environment::domain::singleton::file();

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
               auto archive = serialize::json::writer();
               archive << model;
               archive.consume( output);
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
