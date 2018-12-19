//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "xatmi/executable.h"
#include "xatmi/internal/log.h"
#include "xatmi/internal/transform.h"

#include "common/executable/start.h"

#include "common/functional.h"
#include "common/exception/handle.h"
#include "common/process.h"
#include "common/signal.h"


#include <vector>

namespace casual
{
   namespace xatmi
   {
      namespace executable
      {
         namespace local
         {
            namespace
            {
               int start( const casual_executable_arguments& argument) noexcept
               {
                  try
                  {
                     casual::xatmi::Trace trace{ "casual::xatmi::executable::local::start"};

                     // We block child so users can spawn stuff without actions/errors from casual
                     common::signal::thread::scope::Block block( { common::signal::Type::child});

                     return common::executable::start( 
                        xatmi::transform::resources( argument.xa_switches),
                        [&](){
                           return common::invoke( argument.entrypoint, argument.argc, argument.argv);
                        }
                     );
                  }
                  catch( ...)
                  {
                     return casual::common::exception::handle();
                  }
                  return 0;
               }

            } // <unnamed>
         } // local
      } // executable
   } // xatmi
} // casual


int casual_run_executable( struct casual_executable_arguments* arguments)
{
    return casual::xatmi::executable::local::start( *arguments);
}








