//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "administration/unittest/cli/command.h"

#include "common/environment/expand.h"
#include "common/environment.h"
#include "common/signal.h"
#include "common/result.h"

#include <string>

namespace casual
{
   using namespace common;

   namespace administration::unittest::cli::command
   {
      Pipe::Pipe( std::string command) : m_old_path{ environment::variable::get( "PATH", std::string{})}
      {
         // sink child signals 
         // TODO should be a scoped registration that restores the handler, if any.
         signal::callback::registration< code::signal::child>( [](){});

         environment::variable::set( "PATH", environment::expand( string::compose( "${CASUAL_MAKE_SOURCE_ROOT}/middleware/administration/bin:$PATH")));
         m_stream = posix::result( ::popen( command.data(), "r"));
      }

      Pipe::Pipe( Pipe&& other) : m_stream{ std::exchange( other.m_stream, m_stream)} {}
      Pipe& Pipe::operator = ( Pipe&& other) { std::exchange( other.m_stream, m_stream); return *this;}

      Pipe::~Pipe() 
      {
         if( ! m_stream)
            return;
         
         ::pclose( m_stream);
         
         if( m_old_path.empty())
            environment::variable::unset( "PATH");
         else
            environment::variable::set( "PATH", m_old_path);
      }

      std::string Pipe::string() &&
      {
         // block all signals but terminate
         signal::thread::scope::Block block{ signal::set::filled( code::signal::terminate)};

         std::array< char, 256> buffer;
         std::ostringstream result;
         auto descriptor = ::fileno( m_stream);

         while( true)
         {
            auto read = posix::result( ::read( descriptor, buffer.data(), buffer.size()));

            if( read == 0)
               return std::move( result).str();

            result.write( buffer.data(), read);
         }
      }

   } // administration::unittest::cli::command
} // casual