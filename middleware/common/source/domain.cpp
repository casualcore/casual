//!
//! casual 
//!

#include "common/domain.h"

#include "common/environment.h"
#include "common/log.h"

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


         std::ostream& operator << ( std::ostream& out, const Identity& value)
         {
            return out << "{ id: " << value.id << ", name: " << value.name << "}";
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
                     environment::variable::get( environment::variable::name::domain::id(), "00000000000000000000000000000000"),
                     environment::variable::get( environment::variable::name::domain::name(), "")};
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
            environment::variable::set( environment::variable::name::domain::name(), local::identity().name);
            environment::variable::set( environment::variable::name::domain::id(), uuid::string( local::identity().id));

         }

         namespace singleton
         {
            file::scoped::Path create( const process::Handle& process, const Identity& identity)
            {
               Trace trace{ "common::domain::singleton::create"};


               auto path = environment::domain::singleton::file();

               auto temp_file = file::scoped::Path{ file::name::unique( path, ".tmp")};

               std::ofstream output( temp_file);

               if( output)
               {
                  output << process.queue << '\n';
                  output << process.pid << '\n';
                  output << identity.name << '\n';
                  output << identity.id << std::endl;

                  log::debug << "domain information - id: " << identity << " - process: " << process << '\n';
               }
               else
               {
                  throw common::exception::invalid::File( "failed to write temporary domain singleton file: " + temp_file.path());
               }


               if( common::file::exists( path))
               {
                  //
                  // There is potentially a running casual-domain already - abort
                  //
                  throw common::exception::invalid::Process( "can only be one casual-domain running in a domain - domain lock file: " + path);
               }

               //
               // Set domain-process so children easy can send messages to us.
               //
               environment::variable::process::set(
                     environment::variable::name::ipc::domain::manager(),
                     process);

               domain::identity( identity);

               common::file::move( temp_file, path);

               temp_file.release();

               return { std::move( path)};
            }

            Result read()
            {
               return read( process::pattern::Sleep{
                  { std::chrono::milliseconds{ 10}, 10},
                  { std::chrono::milliseconds{ 100}, 10},
                  { std::chrono::seconds{ 1}, 60}
               });
            }

            Result read( process::pattern::Sleep retries)
            {
               Trace trace{ "common::domain::singleton::read"};

               log::debug << "retries: " << retries << '\n';

               do
               {
                  std::ifstream file{ common::environment::domain::singleton::file()};

                  if( file)
                  {
                     Result result;
                     {
                        file >> result.process.queue;
                        file >> result.process.pid;
                        file >> result.identity.name;
                        std::string uuid;
                        file >> uuid;
                        result.identity.id = Uuid{ uuid};
                     }

                     environment::variable::process::set( environment::variable::name::ipc::domain::manager(), result.process);
                     common::domain::identity( result.identity);

                     log::debug << "domain information - id: " << result.identity << ", process: " << result.process << '\n';

                     return result;
                  }
               }
               while( retries());

               return {};
            }

         } // singleton

      } // domain
   } // common
} // casual
