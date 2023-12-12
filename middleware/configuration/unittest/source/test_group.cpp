//! 
//! Copyright (c) 2023, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"
#include "common/unittest/file.h"

#include "configuration/group.h"

#include "configuration/model.h"
#include "configuration/model/load.h"


namespace casual
{
   using namespace common;

      namespace local
      {
         namespace
         {
            auto coordinator = []( auto content)
            {
               auto path = unittest::file::temporary::content( ".yaml", std::move( content));
               auto model = configuration::model::load( { path});
               auto coordinator = configuration::group::Coordinator{};
               coordinator.update( model.domain.groups);
               EXPECT_EQ( coordinator.config(), model.domain.groups);
               return coordinator;
            };

         }
      }

   namespace configuration
   {
      TEST( test_group, coordinator__member_of_enabled_group__expect_enabled)
      {
         constexpr auto configuration = R"(
domain:
   groups:
      -  name: a
         enabled: true
)";

         auto coordinator = local::coordinator( configuration);

         EXPECT_TRUE( coordinator.enabled( { "a"}));
      }

      TEST( test_group, coordinator__member_of_disabled_group__expect_disabled)
      {
         constexpr auto configuration = R"(
domain:
   groups:
      -  name: a
         enabled: false
)";

         auto coordinator = local::coordinator( configuration);

         EXPECT_FALSE( coordinator.enabled( { "a"}));
      }

      TEST( test_group, coordinator__member_of_group_with_direct_dependency_enabled__expect_enabled)
      {
         constexpr auto configuration = R"(
domain:
   groups:
      -  name: a
         enabled: true
      -  name: b
         enabled: true
         dependencies:
            - a
)";

         auto coordinator = local::coordinator( configuration);

         EXPECT_TRUE( coordinator.enabled( { "b"}));
      }

      TEST( test_group, coordinator__member_of_group_with_direct_dependency_disabled__expect_disabled)
      {
         constexpr auto configuration = R"(
domain:
   groups:
      -  name: a
         enabled: false
      -  name: b
         enabled: true
         dependencies:
            - a
)";

         auto coordinator = local::coordinator( configuration);

         EXPECT_FALSE( coordinator.enabled( { "b"}));
      }

      TEST( test_group, coordinator__member_of_group_with_transitive_dependency_enabled__expect_enabled)
      {
         constexpr auto configuration = R"(
domain:
   groups:
      -  name: a
         enabled: true
      -  name: b
         enabled: true
         dependencies:
            - a
      -  name: c
         enabled: true
         dependencies:
            - b
)";

         auto coordinator = local::coordinator( configuration);

         EXPECT_TRUE( coordinator.enabled( { "c"}));
      }

      TEST( test_group, coordinator__member_of_group_with_transitive_dependency_disabled__expect_disabled)
      {
         constexpr auto configuration = R"(
domain:
   groups:
      -  name: a
         enabled: false
      -  name: b
         enabled: true
         dependencies:
            - a
      -  name: c
         enabled: true
         dependencies:
            - b
)";

         auto coordinator = local::coordinator( configuration);

         EXPECT_FALSE( coordinator.enabled( { "c"}));
      }

      TEST( test_group, coordinator__member_of_group_with_circular_dependencies__all_enabled__expect_enabled)
      {
         constexpr auto configuration = R"(
domain:
   groups:
      -  name: a
         enabled: true
         dependencies:
            - c
      -  name: b
         enabled: true
         dependencies:
            - a
      -  name: c
         enabled: true
         dependencies:
            - b
      -  name: d
         enabled: true
         dependencies:
            - c
)";

         auto coordinator = local::coordinator( configuration);

         EXPECT_TRUE( coordinator.enabled( { "d"}));
      }

      TEST( test_group, coordinator__member_of_group_with_circular_dependencies__one_disabled__expect_disabled)
      {
         constexpr auto configuration = R"(
domain:
   groups:
      -  name: a
         enabled: false
         dependencies:
            - c
      -  name: b
         enabled: true
         dependencies:
            - a
      -  name: c
         enabled: true
         dependencies:
            - b
      -  name: d
         enabled: true
         dependencies:
            - c
)";

         auto coordinator = local::coordinator( configuration);

         EXPECT_FALSE( coordinator.enabled( { "d"}));
      }

      TEST( test_group, coordinator__multiple_memberships__all_enabled__expect_enabled)
      {
         constexpr auto configuration = R"(
domain:
   groups:
      -  name: a
         enabled: true
      -  name: b
         enabled: true
)";

         auto coordinator = local::coordinator( configuration);

         EXPECT_TRUE( coordinator.enabled( { "a", "b"}));
      }

      TEST( test_group, coordinator__multiple_memberships__one_disabled__expect_disabled)
      {
         constexpr auto configuration = R"(
domain:
   groups:
      -  name: a
         enabled: true
      -  name: b
         enabled: false
)";

         auto coordinator = local::coordinator( configuration);

         EXPECT_FALSE( coordinator.enabled( { "a", "b"}));
      }

      TEST( test_group, coordinator__multiple_memberships_with_dependencies__all_enabled__expect_enabled)
      {
         constexpr auto configuration = R"(
domain:
   groups:
      -  name: a
         enabled: true
      -  name: b
         enabled: true
         dependencies:
            - a
      -  name: c
         enabled: true
      -  name: d
         enabled: true
         dependencies:
            - c
)";

         auto coordinator = local::coordinator( configuration);

         EXPECT_TRUE( coordinator.enabled( { "b", "d"}));
      }

      TEST( test_group, coordinator__multiple_memberships_with_dependencies__one_disabled__expect_disabled)
      {
         constexpr auto configuration = R"(
domain:
   groups:
      -  name: a
         enabled: true
      -  name: b
         enabled: true
         dependencies:
            - a
      -  name: c
         enabled: false
      -  name: d
         enabled: true
         dependencies:
            - c
)";

         auto coordinator = local::coordinator( configuration);

         EXPECT_FALSE( coordinator.enabled( { "b", "d"}));
      }

   } // configuration
} // casual