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


               const state::Batch& find_batch( const std::vector< state::Batch>& bootorder, const std::string& group)
               {
                  return common::range::front( common::range::find_if( bootorder, [&group]( const state::Batch& b){
                     return b.group.get().name == group;
                  }));
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
            auto& global = local::find_batch( bootorder, "global");

            EXPECT_TRUE( global.executables.size() == 1) << "global: " << global;
            auto& executable = global.executables.at( 0);
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

            {
               auto& batch = local::find_batch( bootorder, "global");
               ASSERT_TRUE( batch.executables.size() == 1) << "state: " << state << "\nbatch: " << batch;
               EXPECT_TRUE( batch.executables.at( 0).get().path == "exe2");
            }
            {
               auto& batch = local::find_batch( bootorder, "group_1");
               ASSERT_TRUE( batch.executables.size() == 1);
               EXPECT_TRUE( batch.executables.at( 0).get().path == "exe1");
            }
         }


      } // manager

   } // domain


} // casual
