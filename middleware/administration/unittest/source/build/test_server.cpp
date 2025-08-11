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

            auto compile_server()
            {
               auto content = R"(
#include <casual/xatmi.h>

extern "C"
{
   void a( TPSVCINFO *context) {}
   void b( TPSVCINFO *context) {}
   void c( TPSVCINFO *context) {}
   void d( TPSVCINFO *context) {}
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
               -  "${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/include"
               -  "${CASUAL_MAKE_SOURCE_ROOT}/middleware/xatmi/include"
            library: 
               -  "${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin"
               -  "${CASUAL_MAKE_SOURCE_ROOT}/middleware/common/bin"
               -  "${CASUAL_MAKE_SOURCE_ROOT}/middleware/xatmi/bin"
)");
            }

            constexpr std::string_view build_server_path = "${CASUAL_MAKE_SOURCE_ROOT}/middleware/tools/bin/casual-build-server";

         } // <unnamed>
      } // local

      TEST( administration_build_server, building_with_arguments)
      {
         common::unittest::Trace trace;
         
         auto output = common::unittest::file::temporary::name( ".server");

         auto object_file = local::compile_server();

         constexpr auto services = "a,b,c,d";
         
         auto capture = administration::unittest::cli::command::execute(
            local::build_server_path, " --service ", services, " --output ", output.string(), " --build-directives ", object_file, " -O3 -I ${CASUAL_MAKE_SOURCE_ROOT}/middleware/xatmi/include -L ${CASUAL_MAKE_SOURCE_ROOT}/middleware/xatmi/bin");

         EXPECT_TRUE( capture) << CASUAL_NAMED_VALUE( capture);

         EXPECT_TRUE( std::filesystem::file_size( output) > 0);
      }

      TEST( administration_build_server, building_resource_with_arguments)
      {
         common::unittest::Trace trace;
         
         auto output = common::unittest::file::temporary::name( ".server");

         auto system = local::system_configuration();

         auto object_file = local::compile_server();

         constexpr auto services = "a,b,c,d";
         
         auto capture = administration::unittest::cli::command::execute(
            local::build_server_path, " --service ", services, " --output ", output.string(), 
            " --system-configuration ", system.string(), 
            " --resource-keys rm-mockup",
            " --build-directives ", object_file, " -O3 -I ${CASUAL_MAKE_SOURCE_ROOT}/middleware/xatmi/include -L ${CASUAL_MAKE_SOURCE_ROOT}/middleware/xatmi/bin");

         EXPECT_TRUE( capture) << CASUAL_NAMED_VALUE( capture);

         EXPECT_TRUE( std::filesystem::file_size( output) > 0);
      }

      TEST( administration_build_server, building_resource_with_configuration_file)
      {
         common::unittest::Trace trace;

         auto configuration = common::unittest::file::temporary::content( ".yaml", R"(
server:
  default:
    service:
      transaction: "join"
      category: "some.category"
  resources:
    - key: "rm-mockup"
      name: "resource-1"
  services:
    - name: "a"
    - name: "b"
      transaction: "auto"
    - name: "foo"
      function: "c"
      visibility: "undiscoverable"
    - name: "bar"
      function: "d"
      category: "some.other.category" 
)");
         
         auto output = common::unittest::file::temporary::name( ".server");

         auto system = local::system_configuration();

         auto object_file = local::compile_server();

         auto capture = administration::unittest::cli::command::execute(
            local::build_server_path, " --definition ", configuration, " --output ", output, 
            " --system-configuration ", system, 
            " --build-directives ", object_file, " -O3 -I ${CASUAL_MAKE_SOURCE_ROOT}/middleware/xatmi/include -L ${CASUAL_MAKE_SOURCE_ROOT}/middleware/xatmi/bin");

         EXPECT_TRUE( capture) << CASUAL_NAMED_VALUE( capture);

         EXPECT_TRUE( std::filesystem::file_size( output) > 0);
      }

   } // administration   
} // casual
