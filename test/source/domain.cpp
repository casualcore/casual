//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "casual/test/domain.h"

#include "common/environment.h"
#include "common/communication/instance.h"
#include "common/mockup/log.h"

namespace casual
{
   using namespace common;
   namespace test
   {
      namespace domain
      {
         namespace local
         {
            namespace
            {
               std::vector< file::scoped::Path> files( file::scoped::Path&& file)
               {
                  std::vector< file::scoped::Path> result;
                  result.push_back( std::move( file));
                  return result;
               }

               void environment( const std::string& home) 
               {
                  // make sure we use our newly built stuff in the repo
                  environment::variable::set( "CASUAL_HOME", "./home");
                  environment::variable::set( "CASUAL_DOMAIN_HOME", home);

                  // create directores for nginx...
                  common::directory::create( home + "/logs");

                  //! resets the environment
                  common::environment::reset();
               }
               
            } // <unnamed>
         } // local
         
         Manager::Manager( std::vector< file::scoped::Path> files)
            : m_files{ std::move( files)},
               m_process{ "./home/bin/casual-domain-manager", {
               "--event-ipc", string::compose( communication::ipc::inbound::ipc()),
               "--configuration-files", string::join( m_files, " "),
               "--persist", "false"
            }}
         {
            // Wait for the domain to boot
            m_process.handle( unittest::domain::manager::wait( communication::ipc::inbound::device()));

            log::line( mockup::log, "domain-manager is running with home: ", m_preconstruct.home);
         }

         Manager::Manager( file::scoped::Path file)
            : Manager( local::files( std::move( file)))
         {
         }

         Manager::~Manager()
         {
            if( file::exists( m_preconstruct.home))
            {
               file::remove( m_preconstruct.home);
            }
         }

         void Manager::activate()
         {
            log::line( mockup::log, "activates domain: ", m_process.handle(), " - with home: ", m_preconstruct.home);

            local::environment( m_preconstruct.home);
         }

         Manager::Preconstruct::Preconstruct() 
         {
            local::environment( home);

            
         }

      } // domain
   } // test
} // casual