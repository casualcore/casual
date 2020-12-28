//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/exception/guard.h"
#include "common/argument.h"
#include "casual/buffer/admin/cli.h"

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
                  void main( int argc, char** argv)
                  {
                     std::string type = common::buffer::type::x_octet();

                     constexpr auto information = R"([deprecated] use `casual buffer --compose` instead)";

                     argument::Parse{ information,
                        argument::Option{ std::tie( type), { "--type"}, "type of the composed buffer"}
                     }( argc, argv);

                     std::cerr << information << '\n';

                     casual::buffer::admin::cli::detail::compose( std::move( type));
                  }
                  
               } // <unnamed>
            } // local
         } // generic

      } // tools
   } // buffer
   
} // casual


int main( int argc, char** argv)
{
   return casual::common::exception::main::guard( [&]()
   {
      casual::buffer::tools::generic::local::main( argc, argv);
   });
} 