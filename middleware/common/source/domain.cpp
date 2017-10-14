//!
//! casual 
//!

#include "common/domain.h"

#include "common/environment.h"
#include "common/log.h"
#include "common/exception/system.h"
#include "common/string.h"

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
                  throw exception::system::invalid::File( "failed to write temporary domain singleton file: " + temp_file.path());
               }


               if( common::file::exists( path))
               {
                  auto content = singleton::read( path);

                  log::debug << "domain: " << content << '\n';

                  //
                  // There is potentially a running casual-domain already - abort
                  //
                  throw exception::system::invalid::Process( string::compose( 
                     "can only be one casual-domain running in a domain - domain lock file: ", path, " content: ", content));
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

            std::ostream& operator << ( std::ostream& out, const Result& value)
            {
               return out << "{ process: " << value.process
                     << ", domain: " << value.identity
                     << '}';
            }


            Result read( const std::string& path, process::pattern::Sleep retries)
            {
               Trace trace{ "common::domain::singleton::read"};

               log::debug << "path: " << path << "retries: " << retries << '\n';

               do
               {
                  std::ifstream file{ path};

                  if( file)
                  {
                     Result result;
                     {
                        auto queue = result.process.queue.value();
                        file >> queue;
                        result.process.queue = strong::ipc::id{ queue};
                        
                        auto pid = result.process.pid.value();
                        file >> pid;
                        result.process.pid = strong::process::id{ pid};

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
            Result read( process::pattern::Sleep retries)
            {
               return read( common::environment::domain::singleton::file(), std::move( retries));
            }
            
            Result read( const std::string& path)
            {
               return read( path, process::pattern::Sleep{
                  { std::chrono::milliseconds{ 100}, 10}
               });
            }

            Result read()
            {
               return read( common::environment::domain::singleton::file());
            }

         } // singleton

      } // domain
   } // common
} // casual
