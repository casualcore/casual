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
   namespace configuration
   {
      namespace group
      {
         void Coordinator::update( const std::vector< model::domain::Group>& configured_groups)
         {
            auto groups = algorithm::transform( configured_groups, []( const auto& group)
            {
               return Group{ group.name, group.note, group.enabled, group.dependencies};
            });

            m_groups = std::move( groups);
         }

         namespace local
         {
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
         }

         bool Coordinator::enabled( const std::vector< std::string>& memberships) const
         {
            return local::enabled( memberships, m_groups);
         }

         std::vector< model::domain::Group> Coordinator::config() const
         {
            return algorithm::transform( m_groups, []( const auto& group)
            {
               model::domain::Group result;
               result.name = group.name;
               result.note = group.note;
               result.enabled = group.enabled;
               result.dependencies = group.dependencies;
               return result;
            });
         }

      } // group
   } // configuration
} // casual