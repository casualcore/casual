//!
//! casual 
//!

#include "domain/domain.h"

#include "common/file.h"
#include "common/environment.h"
#include "common/uuid.h"


#include <fstream>

namespace casual
{
   using namespace common;

   namespace domain
   {

      namespace local
      {
         namespace
         {

            file::scoped::Path singleton()
            {
               auto path = environment::domain::singleton::file();
               std::ifstream file( path);

               if( file)
               {
                  //
                  // There is potentially a running casual-domain already - abort
                  //
                  throw common::exception::invalid::Process( "can only be one casual-domain running in a domain");
               }

               file::scoped::Path result( std::move( path));


               std::ofstream output( result);

               if( output)
               {
                  output << communication::ipc::inbound::id() << std::endl;
                  output << process::uuid() << std::endl;
               }
               else
               {
                  throw common::exception::invalid::File( "failed to write process singleton queue file: " + path);
               }

               return result;
            }


         } // <unnamed>
      } // local


      Domain::Domain( Settings&& settings)
        : m_singelton{ local::singleton()}
      {
         //
         // Set our ipc-queue so children easy can send messages to us.
         //
         environment::variable::set( environment::variable::name::domain::ipc(), communication::ipc::inbound::id());


      }

      void Domain::start()
      {

      }

   } // domain

} // casual
