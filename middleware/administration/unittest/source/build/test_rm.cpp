//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "administration/unittest/cli/command.h"

#include "common/unittest/file.h"

namespace casual
{
   namespace administration
   {

      TEST( administration_build_rm, building_with_rm_mockup__expect_success)
      {
         common::unittest::Trace trace;

         auto system = common::unittest::file::temporary::content( ".yaml", R"(
system:
   resources:
      -  key: rm-mockup
         server: "not-used"
         xa_struct_name: casual_mockup_xa_switch_static
         libraries:
            -  casual-mockup-rm
         paths:
            include:
               -  "${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/include"
               -  "${CASUAL_MAKE_SOURCE_ROOT}/middleware/xatmi/include"
            library: 
               -  "${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin"
               -  "${CASUAL_MAKE_SOURCE_ROOT}/middleware/common/bin"
               -  "${CASUAL_MAKE_SOURCE_ROOT}/middleware/xatmi/bin"
)");
         
         auto output = common::unittest::file::temporary::name( ".exe");

         auto build_rm_path = "${CASUAL_MAKE_SOURCE_ROOT}/middleware/tools/bin/casual-build-resource-proxy";
         
         auto capture = administration::unittest::cli::command::execute(
            build_rm_path, " --output ", output.string(), " --resource-key rm-mockup --system-configuration ", system.string(), " --compile-directives -O3");

         EXPECT_TRUE( capture) << CASUAL_NAMED_VALUE( capture);

         EXPECT_TRUE( std::filesystem::file_size( output) > 0);
      }

      
   } // administration
   
} // casual