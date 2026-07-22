//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "administration/unittest/cli/command.h"
#include "administration/unittest/build/compile.h"

#include "common/unittest/file.h"

namespace casual
{
   namespace administration
   {

      namespace local
      {
         namespace
         {

            
            auto compile_executable()
            {
               auto content = R"(
#include <casual/xatmi.h>

extern "C"
{
   int entry_point( int argc, char* argv[])
   {
      return 0;
   }
}
               )";

               return unittest::build::compile( content);
            }

            auto system_configuration()
            {
               return common::unittest::file::temporary::content( ".yaml", R"(
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
            }

            constexpr std::string_view build_executable_path = "${CMAKE_BINARY_DIR}/middleware/tools/bin/casual-build-executable";

         } // <unnamed>
      } // local


      TEST( administration_build_executable, building_resource_with_configuration_file)
      {
         common::unittest::Trace trace;

         auto configuration = common::unittest::file::temporary::content( ".yaml", R"(
executable:
   resources:
      -  key: "rm-mockup"
         name: "resource-1"
   entrypoint: "entry_point" 

)");
         
         auto output = common::unittest::file::temporary::name( ".server");

         auto system = local::system_configuration();

         auto object_file = local::compile_executable();

         auto capture = administration::unittest::cli::command::execute(
            local::build_executable_path, " --verbose --definition ", configuration, " --output ", output, 
            " --system-configuration ", system, 
            " --build-directives ", object_file, " -O3 -I ${CMAKE_SOURCE_DIR}/middleware/xatmi/include -L ${CMAKE_BINARY_DIR}/middleware/xatmi/bin");

         EXPECT_TRUE( capture) << CASUAL_NAMED_VALUE( capture);

         EXPECT_TRUE( std::filesystem::file_size( output) > 0);
      }
      
   } // administration   
} // casual
