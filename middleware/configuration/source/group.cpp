//!
//! Copyright (c) 2023, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "configuration/group.h"

#include "common/algorithm.h"
#include "common/algorithm/container.h"

#include <vector>

namespace casual
{
   using namespace common;
   namespace configuration::group
   {
      namespace local
      {
         namespace
         {
            auto transform_groups( auto& configured_groups)
            {
               return algorithm::transform( configured_groups, []( const auto& group)
               {
                  return Group{ 
                     .name = group.name, 
                     .enabled = group.enabled, 
                     .dependencies = group.dependencies, 
                     .note = group.note};
               });
            }

            bool enabled( const std::vector< std::string>& memberships, std::vector< Group> groups)
            {
               // if we've checked all our dependencies we're done
               if( memberships.empty() || groups.empty())
                  return true;

               auto [ dependencies, remaining_groups] = algorithm::partition( groups, [ &memberships]( const auto& group)
               {
                  return predicate::boolean( algorithm::find( memberships, group.name));
               });

               // if we find a disabled dependency we're done
               if( algorithm::any_of( dependencies, []( const auto& group){ return !group.enabled;}))
                  return false;
               
               auto transitive_dependencies = algorithm::accumulate( dependencies, std::vector< std::string>{}, []( auto result, const auto& group)
               {
                  algorithm::append_unique( group.dependencies, result);

                  return result;
               });

               return enabled( transitive_dependencies, algorithm::container::vector::create( remaining_groups));
            }
         } // <unnamed>
      } // local

      Coordinator::Coordinator( const std::vector< model::domain::Group>& configured_groups)
         : m_groups{ local::transform_groups( configured_groups)}
      {}

      void Coordinator::update( const std::vector< model::domain::Group>& configured_groups)
      {
         m_groups = local::transform_groups( configured_groups);
      }

      bool Coordinator::enabled( const std::vector< std::string>& memberships) const
      {
         return local::enabled( memberships, m_groups);
      }

      std::vector< model::domain::Group> Coordinator::config() const
      {
         return algorithm::transform( m_groups, []( const auto& group)
         {
            return model::domain::Group{
               .name = group.name,
               .enabled = group.enabled,
               .dependencies = group.dependencies,
               .note = group.note,
            };
         });
      }
   } // configuration::group
} // casual