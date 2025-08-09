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
#include "common/posix.h"

#include <string>

namespace casual
{
   using namespace common;

   namespace administration::unittest::cli::command
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
                     log::debug( "unittest::cli::command::execute - ", code::signal::child, " discarded");
                  });

               }
            } // signal
         } // <unnamed>
      } // local

      namespace detail
      {
         Capture execute( std::string command)
         {
            // use the non blocking.
            return non::blocking::capture( non::blocking::detail::execute( std::move( command))); 
         }

      } // detail


      namespace non::blocking
      {
         namespace detail
         {
            Execution execute( std::string command)
            {
               Trace trace{ "administration::unittest::cli::command::non::blocking::detail::execute"};

               // make sure we've got casual stuff in the path
               auto path = environment::expand( string::compose( "PATH=",
                  "${CMAKE_BINARY_DIR}/middleware/administration/bin:",
                  "${CMAKE_BINARY_DIR}/middleware/domain/bin:",
                  "${CMAKE_BINARY_DIR}/middleware/queue/bin:",
                  "${CMAKE_BINARY_DIR}/middleware/transaction/bin:",
                  "${CMAKE_BINARY_DIR}/middleware/service/bin:",
                  "${CMAKE_BINARY_DIR}/middleware/gateway/bin:",
                  "${PATH}"
               ));
               
               // ignore child signals
               auto guard = local::signal::handler();

               auto shell = common::environment::variable::get( "SHELL").value_or( "sh");
               return common::process::non::blocking::execute( shell, std::vector< std::string>{ "-c", std::move( command)}, { std::move( path)});
            }
         } // detail

         Capture capture( Execution&& execution)
         {
            // ignore child signals
            auto guard = local::signal::handler();

            return common::process::non::blocking::capture( std::move( execution));
         }

         
      } // non::blocking

   } // administration::unittest::cli::command
} // casual
