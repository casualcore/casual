//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "casual/buffer/internal/field.h"
#include "casual/buffer/internal/common.h"
#include "casual/buffer/admin/cli.h"
#include "common/exception/guard.h"

#include "common/argument.h"



#include <iostream>

namespace casual
{
   namespace local
   {
      namespace
      {
         void main(int argc, char **argv)
         {
            std::string format;

            constexpr auto information = R"([deprecated] use `casual buffer --field-to-human` instead)";

            {
               common::argument::Parse{ information,
                  common::argument::Option( std::tie( format), { "--format"}, "which format to transform to")
               }( argc, argv);
            }

            std::cerr << information << '\n';
            buffer::admin::cli::detail::field::to_human( std::move( format));

         }
      } // <unnamed>
   } // local
} // casual

int main(int argc, char **argv)
{
   return casual::common::exception::main::log::guard( [&]()
   {
      casual::local::main( argc, argv);
   });
}
