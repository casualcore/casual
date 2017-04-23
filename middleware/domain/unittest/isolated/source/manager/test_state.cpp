//!
//! casual 
//!

#include <gtest/gtest.h>
#include "common/unittest.h"

#include "domain/manager/state.h"
#include "domain/manager/configuration.h"
#include "domain/manager/manager.h"
#include "domain/manager/persistent.h"
#include "domain/transform.h"

#include "configuration/example/domain.h"

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
                  auto file = common::mockup::file::temporary::content( ".yaml", configuration);

                  Settings settings;
                  settings.configurationfiles.push_back( file);

                  return configuration::state( settings);
               }


               const state::Batch& find_batch( const State& state, const std::vector< state::Batch>& bootorder, const std::string& group)
               {
                  return common::range::front( common::range::find_if( bootorder, [&]( const state::Batch& b){
                     return state.group( b.group).name == group;
                  }));
               }

            } // <unnamed>
         } // local

         TEST( domain_state_boot_order, empty_state___expect_empty_boot_order)
         {
            common::unittest::Trace trace;
            State state;

            EXPECT_TRUE( state.bootorder().empty());

         }

         TEST( domain_state_boot_order, executable_1___expect_1_boot_order)
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
            auto& global = local::find_batch( state, bootorder, ".global");

            EXPECT_TRUE( global.executables.size() == 1) << ".global: " << global;
            auto& executable = global.executables.at( 0);
            EXPECT_TRUE( state.executable( executable.id).path == "echo" );
         }

         TEST( domain_state_boot_order, group_1_exe1__global_exe2__expect__exe2_exe1)
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
               auto& batch = local::find_batch( state, bootorder, ".global");
               ASSERT_TRUE( batch.executables.size() == 1) << "state: " << state << "\nbatch: " << batch;
               EXPECT_TRUE( state.executable( batch.executables.at( 0).id).path == "exe2");
            }
            {
               auto& batch = local::find_batch( state, bootorder, "group_1");
               ASSERT_TRUE( batch.executables.size() == 1);
               EXPECT_TRUE(state.executable( batch.executables.at( 0).id).path == "exe1");
            }
         }

         namespace local
         {
            namespace
            {
               namespace persistent
               {
                  State serialization( const State& state)
                  {
                     common::file::scoped::Path file{
                           common::file::name::unique( common::directory::temporary() + "/state_", ".json")};

                     //EXPECT_TRUE( false) << "file: " << file;

                     manager::persistent::state::save( state, file);
                     return manager::persistent::state::load( file);
                  }

               } // persistent

               struct Equal
               {
                  bool operator () ( const State& lhs, const State& rhs) const
                  {
                     auto tie = []( const State& s){
                        return std::tie(
                              s.manager_id,
                              s.group_id.gateway,
                              s.group_id.global,
                              s.group_id.master,
                              s.group_id.queue,
                              s.group_id. transaction
                           );
                     };

                     return tie( lhs) == tie( rhs);
                  }
               }; // equal

               bool equal( const State& lhs, const State& rhs)
               {
                  return Equal{}( lhs, rhs);
               }

            } // <unnamed>
         } // local

         TEST( domain_state_persistent, empty)
         {
            common::unittest::Trace trace;

            State state;
            auto persisted = local::persistent::serialization( state);

            EXPECT_TRUE( persisted.manager_id == state.manager_id);
         }

         TEST( domain_state_persistent, group_id)
         {
            common::unittest::Trace trace;


            State state;
            {
               state.group_id.master = state::Group::id_type{ 10};
               state.group_id.transaction = state::Group::id_type{ 11};
               state.group_id.queue = state::Group::id_type{ 12};
               state.group_id.global = state::Group::id_type{ 13};
               state.group_id.gateway = state::Group::id_type{ 14};
            }

            auto persisted = local::persistent::serialization( state);

            EXPECT_TRUE( local::equal( state, persisted));
         }

         TEST( domain_state_persistent, from_example_domain)
         {
            common::unittest::Trace trace;

            auto state = transform::state( casual::configuration::example::domain());

            auto persisted = local::persistent::serialization( state);

            EXPECT_TRUE( local::equal( state, persisted));
         }

      } // manager

   } // domain


} // casual
