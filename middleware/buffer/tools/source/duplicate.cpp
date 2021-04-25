//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "casual/buffer/admin/cli.h"

#include "common/exception/guard.h"
#include "common/argument.h"


namespace casual
{
   using namespace common;

   namespace buffer
   {
      namespace tools
      {
         namespace local
         {
            namespace
            {
               void main( int argc, char** argv)
               {
                  platform::size::type count = 1;

                  constexpr auto information = R"([deprecated] use `casual buffer --duplicate` instead)";

                  argument::Parse{ information,
                     argument::Option{ std::tie( count), { "--count"}, "number of 'duplications', applied for each buffer"}
                  }( argc, argv);

                  std::cerr << information << '\n';
                  
                  casual::buffer::admin::cli::detail::duplicate( count);
               } 

            } // <unnamed>
         } // local
      } // tools
   } // buffer
} // casual

int main( int argc, char** argv)
{
   return casual::common::exception::main::log::guard( [&]()
   {
      casual::buffer::tools::local::main( argc, argv);
   });
} 