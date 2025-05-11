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
      using Capture = common::process::Capture;

      namespace detail
      { 
         Capture execute( std::string command);
      } // detail

      template< typename... Cs>
      Capture execute( Cs&&... commands)
      {
         return detail::execute( common::string::compose( std::forward< Cs>( commands)...));
      }
   
   } // administration::unittest::cli::command
} // casual