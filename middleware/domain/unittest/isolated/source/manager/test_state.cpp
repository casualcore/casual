//!
//! casual 
//!

#include <gtest/gtest.h>
#include "common/unittest.h"

#include "domain/manager/state.h"
#include "domain/manager/configuration.h"
#include "domain/manager/manager.h"


#include "common/mockup/file.h"

namespace casual
{

   namespace domain
   {
      namespace manager
      {
         namespace local
         {
            namespace
            {
               State configure( const std::string configuration)
               {
                  auto file = common::mockup::file::temporary( ".yaml", configuration);

                  Settings settings;
                  settings.configurationfiles.push_back( file);

                  return configuration::state( settings);
               }


            } // <unnamed>
         } // local

         TEST( casual_domain_state_boot_order, empty_state___expect_empty_boot_order)
         {
            common::unittest::Trace trace;
            State state;

            EXPECT_TRUE( state.bootorder().empty());

         }

         TEST( casual_domain_state_boot_order, executable_1___expect_1_boot_order)
         {
            common::unittest::Trace trace;
            auto state = local::configure( R"(
domain:

  name: unittest-domain

  executables:
    - path: echo
      arguments: [poop]
      instances: 10

)" );

            auto bootorder = state.bootorder();
            ASSERT_TRUE( bootorder.size() == 2) << "bootorder: " << common::range::make( bootorder);
            EXPECT_TRUE( bootorder.at( 0).executables.size() == 1) << "bootorder: " << common::range::make( bootorder);
            auto& executable = bootorder.at( 0).executables.at( 0);
            EXPECT_TRUE( executable.get().path == "echo" );
         }

         TEST( casual_domain_state_boot_order, group_1_exe1__global_exe2__expect__exe2_exe1)
         {
            common::unittest::Trace trace;
            auto state = local::configure( R"(
domain:
  
  name: unittest-domain

  groups:
    - name: group_1

  executables:
    - path: exe1
      memberships: [ group_1]

    - path: exe2

)" );

            auto bootorder = state.bootorder();
            ASSERT_TRUE( bootorder.size() == 3);
            {
               auto& batch = bootorder.at( 0);
               EXPECT_TRUE( batch.group.get().name == "global" ) << "bootorder: " << common::range::make( bootorder);
               ASSERT_TRUE( batch.executables.size() == 1) << "bootorder: " << common::range::make( bootorder);
               EXPECT_TRUE( batch.executables.at( 0).get().path == "exe2");
            }
            {
               auto& batch = bootorder.at( 1);
               EXPECT_TRUE( batch.group.get().name == "group_1" ) << "bootorder: " << common::range::make( bootorder) << " - state: " << state;
               ASSERT_TRUE( batch.executables.size() == 1);
               EXPECT_TRUE( batch.executables.at( 0).get().path == "exe1");
            }
         }


      } // manager

   } // domain


} // casual
