//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/domain.h"

#include "common/environment.h"
#include "common/log.h"
#include "common/exception/system.h"
#include "common/string.h"

// std
#include <fstream>

namespace casual
{
   namespace common
   {
      namespace domain
      {
         Identity::Identity() = default;

         Identity::Identity( const Uuid& id, std::string name)
            : id{ id}, name{ std::move( name)}
         {
         }

         Identity::Identity( std::string name)
            : Identity{ uuid::make(), std::move( name)}
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

               Identity& identity()
               {
                  static Identity id{
                     Uuid{ environment::variable::get( environment::variable::name::domain::id, "00000000000000000000000000000000")},
                     environment::variable::get( environment::variable::name::domain::name, "")};
                  return id;

               }
            }
         }

         const Identity& identity()
         {
            return local::identity();
         }

         void identity( Identity value)
         {
            local::identity() = value;
            if( ! local::identity().id)
            {
               local::identity().id = uuid::make();
            }
            environment::variable::set( environment::variable::name::domain::name, local::identity().name);
            environment::variable::set( environment::variable::name::domain::id, uuid::string( local::identity().id));

         }

         namespace singleton
         {
            file::scoped::Path create( const process::Handle& process, const Identity& identity)
            {
               Trace trace{ "common::domain::singleton::create"};

               auto& path = environment::domain::singleton::file();

               auto temp_file = file::scoped::Path{ file::name::unique( path, ".tmp")};

               std::ofstream output( temp_file);

               if( output)
               {
                  output << process.ipc << '\n';
                  output << process.pid << '\n';
                  output << identity.name << '\n';
                  output << identity.id << '\n';

                  log::line( log::debug, "domain information - id: ", identity, " - process: ", process);
               }
               else
               {
                  throw exception::system::invalid::File( "failed to write temporary domain singleton file: " + temp_file.path());
               }


               if( common::file::exists( path))
               {
                  auto content = singleton::read( path);

                  log::line( log::debug, "domain: ", content);

                  // There is potentially a running casual-domain already - abort
                  throw exception::system::invalid::Process( string::compose( 
                     "can only be one casual-domain running in a domain - domain lock file: ", path, " content: ", content));
               }

               // Set domain-process so children easy can send messages to us.
               environment::variable::process::set(
                     environment::variable::name::ipc::domain::manager,
                     process);

               domain::identity( identity);

               common::file::move( temp_file, path);

               temp_file.release();

               return { std::move( path)};
            }


            Result read( const std::string& path)
            {
               Trace trace{ "common::domain::singleton::read"};

               log::line( log::debug, "path: ", path);
  
               std::ifstream file{ path};

               if( file)
               {
                  Result result;
                  {
                     std::string ipc;
                     file >> ipc;
                     result.process.ipc = strong::ipc::id{ Uuid{ ipc}};
                     
                     auto pid = result.process.pid.value();
                     file >> pid;
                     result.process.pid = strong::process::id{ pid};

                     file >> result.identity.name;
                     std::string uuid;
                     file >> uuid;
                     result.identity.id = Uuid{ uuid};
                  }

                  environment::variable::process::set( environment::variable::name::ipc::domain::manager, result.process);
                  common::domain::identity( result.identity);

                  log::line( log::debug, "domain information - id: ", result.identity, ", process: ", result.process);

                  return result;
               }

               return {};
            }

            Result read()
            {
               return read( common::environment::domain::singleton::file());
            }

         } // singleton

      } // domain
   } // common
} // casual
