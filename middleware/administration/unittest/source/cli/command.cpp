//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "administration/unittest/cli/command.h"

#include "common/environment/expand.h"
#include "common/signal.h"
#include "common/result.h"

#include "common/unittest/environment.h"

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
                        log::line( verbose::log, "unittest::cli::command - ", code::signal::child, " discarded");
                     });

                  }
               } // signal
            } // <unnamed>
         } // local

         struct Pipe::Implementation
         {
            Implementation( std::string command) 
               : scoped_signal_handler{ local::signal::handler()}, 
               old_path{ environment::variable::get( "PATH").value_or( "")}
            {
               command = environment::expand( std::move( command));

               common::unittest::environment::variable::set( "PATH", environment::expand( "${CASUAL_MAKE_SOURCE_ROOT}/middleware/administration/bin:${PATH}"));
               log::line( verbose::log, "PATH: ", environment::variable::get( "PATH"));
               
               log::line( verbose::log, "executing command: ", command);
               stream = posix::result( ::popen( command.data(), "r"));
            }

            std::string consume()
            {
               // block all signals but terminate
               signal::thread::scope::Block block{ signal::set::filled( code::signal::terminate)};

               std::array< char, 256> buffer;
               std::ostringstream result;
               auto descriptor = ::fileno( stream);

               while( true)
               {
                  auto read = posix::result( ::read( descriptor, buffer.data(), buffer.size()));

                  if( read == 0)
                     return std::move( result).str();

                  result.write( buffer.data(), read);
               }
            }

            ~Implementation()
            {
               ::pclose( stream);
            
               if( old_path.empty())
                  environment::variable::unset( "PATH");
               else
                  common::unittest::environment::variable::set( "PATH", old_path);
            }

            using scoped_signal_handler_type = decltype( local::signal::handler());
            scoped_signal_handler_type scoped_signal_handler; 

            std::string old_path;
            FILE* stream = nullptr;
         };

         Pipe::Pipe( std::string command) 
            : m_implementation{ std::move( command)}
         {}

         Pipe::~Pipe() = default;

         std::string Pipe::consume() &&
         {
            return m_implementation->consume();
         }

         std::string Pipe::string() &&
         {
            return m_implementation->consume();
         }

      } // detail

   } // administration::unittest::cli::command
} // casual