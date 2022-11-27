//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "domain/manager/state.h"
#include "domain/manager/state/order.h"
#include "domain/manager/configuration.h"
#include "domain/manager/transform.h"

#include "configuration/model/load.h"
#include "configuration/example/model.h"

#include "common/unittest/file.h"

#include "serviceframework/log.h"

namespace casual
{
   namespace domain::manager
   {

      namespace local
      {
         namespace
         {
            State configure( const std::string configuration)
            {
               auto file = common::unittest::file::temporary::content( ".yaml", configuration);

               return transform::model( casual::configuration::model::load( { file}));
            }


            const state::dependency::Group& find_batch( const State& state, const std::vector< state::dependency::Group>& bootorder, const std::string& group)
            {
               return common::range::front( common::algorithm::find_if( bootorder, [&]( const state::dependency::Group& dependency){
                  return dependency.description == group;
               }));
            }

         } // <unnamed>
      } // local

      TEST( domain_state_environemnt, no_default__no_explict__expect_0_variables)
      {
         common::unittest::Trace trace;
         auto state = local::configure( R"(
domain:

  name: unittest-domain

  executables:
    - path: echo

)" );

         EXPECT_TRUE( state.variables( state.executables.at( 0)).empty());
      }

      TEST( domain_state_environemnt, no_default__2_explict__expect_2_variables)
      {
         common::unittest::Trace trace;
         auto state = local::configure( R"(
domain:

  name: unittest-domain

  executables:
    - path: echo
      environment:
        variables:
          - key: a
            value: 1
          - key: b
            value: 2

)" );

            EXPECT_TRUE( state.variables( state.executables.at( 0)).size() == 2);
         }

         TEST( domain_state_environemnt, default_2___explict_0___expect_2_variables)
         {
            common::unittest::Trace trace;
            auto state = local::configure( R"(
domain:

  name: unittest-domain

  default:
     environment:
        variables:
          - key: a
            value: 1
          - key: b
            value: 2

  executables:
    - path: echo

)" );

         EXPECT_TRUE( state.variables( state.executables.at( 0)).size() == 2) << CASUAL_NAMED_VALUE( state);
      }

      TEST( domain_state_environemnt, default_2___explict_2___expect_4_variables)
      {
         common::unittest::Trace trace;
         auto state = local::configure( R"(
domain:

  name: unittest-domain

  default:
     environment:
        variables:
          - key: a
            value: 1
          - key: b
            value: 2

  executables:
    - path: echo
      environment:
        variables:
          - { key: c, value: 3}
          - key: d
            value: 4

)" );

         auto result = state.variables( state.executables.at( 0));
         ASSERT_TRUE( result.size() == 4) << CASUAL_NAMED_VALUE( result);
         EXPECT_TRUE( result.at( 0) == "a=1");
         EXPECT_TRUE( result.at( 3) == "d=4");
      }

      TEST( domain_state_boot_order, empty_state___expect_empty_boot_order)
      {
         common::unittest::Trace trace;
         State state;

         EXPECT_TRUE( state::order::boot( state).empty());

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

            auto bootorder = state::order::boot( state);
            auto& global = local::find_batch( state, bootorder, ".global");

            EXPECT_TRUE( global.executables.size() == 1) << CASUAL_NAMED_VALUE( global);
            auto id = global.executables.at( 0);
            EXPECT_TRUE( state.entity( id).path == "echo" );
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


         auto bootorder = state::order::boot( state);

         {
            auto& batch = local::find_batch( state, bootorder, ".global");
            ASSERT_TRUE( batch.executables.size() == 1) << CASUAL_NAMED_VALUE( state) << "\n" << CASUAL_NAMED_VALUE( batch);
            EXPECT_TRUE( state.entity( batch.executables.at( 0)).path == "exe2");
         }
         {
            auto& batch = local::find_batch( state, bootorder, "group_1");
            ASSERT_TRUE( batch.executables.size() == 1);
            EXPECT_TRUE(state.entity( batch.executables.at( 0)).path == "exe1");
         }
      }

      TEST( domain_state_instances, executable_default)
      {
         common::unittest::Trace trace;

         auto executable = state::Executable::create();

         EXPECT_TRUE( executable.spawnable().empty());
         EXPECT_TRUE( executable.shutdownable().empty());
      }

      TEST( domain_state_instances, executable_instance_resize__expect_spawnable)
      {
         common::unittest::Trace trace;

         auto executable = state::Executable::create();
         executable.instances.resize( 5);

         EXPECT_TRUE( executable.spawnable().size() == 5);
         EXPECT_TRUE( executable.shutdownable().empty()) << CASUAL_NAMED_VALUE( executable);
      }

      TEST( domain_state_instances, server_default)
      {
         common::unittest::Trace trace;

         auto server = state::Server::create();

         EXPECT_TRUE( server.spawnable().empty());
         EXPECT_TRUE( server.shutdownable().empty());
      }

      TEST( domain_state_instances, server_instance_resize__expect_spawnable)
      {
         common::unittest::Trace trace;

         auto server = state::Server::create();
         server.instances.resize( 5);

         EXPECT_TRUE( server.spawnable().size() == 5);
         EXPECT_TRUE( server.shutdownable().empty()) << CASUAL_NAMED_VALUE( server);
      }

   } // domain::manager

} // casual
