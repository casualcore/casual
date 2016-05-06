//!
//! casual 
//!

#include "domain/manager/configuration.h"
#include "domain/manager/manager.h"
#include "domain/transform.h"

#include "config/domain.h"

namespace casual
{
   using namespace common;
   namespace domain
   {
      namespace manager
      {
         namespace configuration
         {
            namespace local
            {
               namespace
               {
                  config::domain::Domain configuration( const Settings& settings)
                  {

                     if( settings.configurationfiles.empty())
                     {
                        return config::domain::get();
                     }

                     return range::accumulate(
                           range::transform( settings.configurationfiles, []( const std::string& f){ return config::domain::get( f);}),
                           config::domain::Domain{}
                           );
                  }
               } // <unnamed>
            } // local

            State state( const Settings& settings)
            {
               return transform::state( local::configuration( settings));
            }


         } // configuration
      } // manager
   } // domain



} // casual
