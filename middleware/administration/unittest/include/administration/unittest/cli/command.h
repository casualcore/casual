//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/environment/expand.h"
#include "common/string.h"

#include <string>

namespace casual
{
   namespace administration::unittest::cli::command
   {

      struct Pipe
      {
         template< typename... Cs>
         explicit Pipe( Cs&&... commands) : Pipe{ common::environment::expand( common::string::compose( std::forward< Cs>( commands)...))}
         {}

         Pipe( Pipe&& other);
         Pipe& operator = ( Pipe&& other);

         ~Pipe();

         std::string string() &&;

         template< typename... Cs>
         friend auto execute( Cs&&... commands);

      private:
         explicit Pipe( std::string command);

         std::string m_old_path;
         FILE* m_stream = nullptr;
      };

      template< typename... Cs>
      auto execute( Cs&&... commands)
      {
         return Pipe{ std::forward< Cs>( commands)...};
      }
   
   } // administration::unittest::cli::command
} // casual