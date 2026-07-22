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
               -  "${CMAKE_SOURCE_DIR}/middleware/transaction/include"
               -  "${CMAKE_SOURCE_DIR}/middleware/xatmi/include"
            library: 
               -  "${CMAKE_BINARY_DIR}/middleware/transaction/bin"
               -  "${CMAKE_BINARY_DIR}/middleware/common/bin"
               -  "${CMAKE_BINARY_DIR}/middleware/xatmi/bin"
)");
         
         auto output = common::unittest::file::temporary::name( ".exe");

         auto build_rm_path = "${CMAKE_BINARY_DIR}/middleware/tools/bin/casual-build-resource-proxy";
         
         auto capture = administration::unittest::cli::command::execute(
            build_rm_path, " --verbose --output ", output.string(), " --resource-key rm-mockup --system-configuration ", system.string(), " --compile-directives -O3");

         EXPECT_TRUE( capture) << CASUAL_NAMED_VALUE( capture);

         EXPECT_TRUE( std::filesystem::file_size( output) > 0);
      }

      
   } // administration
   
} // casual