//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "casual/buffer/admin/cli.h"
#include "casual/buffer/internal/common.h"

#include "common/exception/guard.h"

#include "common/argument.h"


#include <iostream>

namespace casual
{

   void main(int argc, char **argv)
   {
      std::string format;

      constexpr auto information = R"([deprecated] use `casual buffer --field-from-human` instead)";

      common::argument::Parse{ information,
         common::argument::Option( std::tie( format), buffer::admin::cli::detail::format::completion(), { "--format"}, "which format to expect on stdin"),
      }( argc, argv);

      std::cerr << information << '\n';

      buffer::admin::cli::detail::field::from_human( std::move( format));
   }

} // casual



int main(int argc, char **argv)
{
   return casual::common::exception::main::log::guard( [&]()
   {
      casual::main( argc, argv);
   });
}
