//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/string/compose.h"
#include "common/pimpl.h"

#include <string>

namespace casual
{
   namespace administration::unittest::cli::command
   {
      namespace detail
      {      
         struct Pipe
         {
            explicit Pipe( std::string command);
            ~Pipe();

            //! @returns the output of the command
            std::string consume() &&;

            //! @deprecated use consume instead
            std::string string() &&;

         private:
            struct Implementation;
            common::move::Pimpl< Implementation> m_implementation;
         };

         static_assert( ! std::is_copy_constructible_v< Pipe> && ! std::is_copy_assignable_v< Pipe>);

      } // detail

      template< typename... Cs>
      auto execute( Cs&&... commands)
      {
         return detail::Pipe{ common::string::compose( std::forward< Cs>( commands)...)};
      }
   
   } // administration::unittest::cli::command
} // casual