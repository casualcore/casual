//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "casual/buffer/admin/cli.h"


#include "common/exception/guard.h"
#include "casual/platform.h"
#include "casual/argument.h"
#include "common/terminal.h"

#include <iostream>

namespace casual
{
   using namespace common;

   namespace buffer
   {
      namespace tools
      {
         namespace generic
         {
            namespace local
            {
               namespace
               {
                  void main( int argc, const char** argv)
                  {
                     bool print_type = false;

                     constexpr auto information = R"([deprecated] use `casual buffer --extract` instead)";

                     argument::parse( information, {
                        argument::Option{ argument::option::flag( print_type), { "--print-type"}, "prints the type of the buffer(s) to stderr"}
                     }, argc, argv);

                     terminal::output::directive().verbose( print_type);

                     std::cerr << information << '\n';
                     
                     casual::buffer::admin::cli::detail::extract();
                  }
                  
               } // <unnamed>
            } // local
         } // generic

      } // tools
   } // buffer
   
} // casual


int main( int argc, const char** argv)
{
   return casual::common::exception::main::log::guard( [&]()
   {
      casual::buffer::tools::generic::local::main( argc, argv);
   });
} 