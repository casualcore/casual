//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "administration/unittest/cli/command.h"

#include "common/environment/expand.h"
#include "common/environment.h"
#include "common/algorithm/container.h"
#include "common/signal.h"
#include "common/result.h"

#include <string>

namespace casual
{
   using namespace common;

   namespace administration::unittest::cli::command
   {
      namespace detail
      {
         namespace local
         {
            namespace
            {
               namespace signal
               {
                  auto handler()
                  {
                     return common::signal::callback::scoped::replace< code::signal::child>( []()
                     {
                        log::line( verbose::log, "unittest::cli::command::execute - ", code::signal::child, " discarded");
                     });

                  }
               } // signal
            } // <unnamed>
         } // local

         Capture execute( std::string command)
         {
            Trace trace{ "administration::unittest::cli::command::execute"};

            // make sure we've got casual stuff in the path
            auto path = string::compose( "PATH=", environment::expand( "${CASUAL_MAKE_SOURCE_ROOT}/middleware/administration/bin:${PATH}")); 

            // ignore child signals
            auto guard = local::signal::handler();

            auto shell = common::environment::variable::get( "SHELL").value_or( "sh");
            return common::process::execute( shell, std::vector< std::string>{ "-c", std::move( command)}, { std::move( path)});
         }

      } // detail

   } // administration::unittest::cli::command
} // casual