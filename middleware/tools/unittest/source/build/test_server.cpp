//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "tools/build/task.h"

namespace casual
{
   namespace tools
   {
      namespace build
      {
         TEST( tools_build_server, add_directive)
         {
            build::Directive directive;

            auto add_directive = build::Directive::split( directive.directives);

            add_directive( "-l a -l b -l c", {});

            const std::vector< std::string> expected{ "-l", "a", "-l", "b", "-l", "c"};

            EXPECT_TRUE( directive.directives == expected) << CASUAL_NAMED_VALUE( directive.directives);
         }

      } // build
   } // tools
} // casual
