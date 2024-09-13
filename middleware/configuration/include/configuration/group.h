//!
//! Copyright (c) 2023, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "configuration/model.h"

#include "common/strong/type.h"

namespace casual
{
   namespace configuration
   {
      struct Group
      {
         // TODO Why do we not just use model::domain::Group?

         std::string name;
         bool enabled;
         std::vector< std::string> dependencies;
         std::string note;

         inline friend bool operator == ( const Group& lhs, const std::string& name) { return lhs.name == name;}

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( name);
            CASUAL_SERIALIZE( enabled);
            CASUAL_SERIALIZE( dependencies);
            CASUAL_SERIALIZE( note);
         );
      };

      namespace group
      {
         struct Coordinator
         {
            Coordinator() = default;
            Coordinator( const std::vector< model::domain::Group>& configured_groups);

            //! evaluates whether an entity with the provided memberships should be enabled
            //! @returns whether the entity is enabled 
            bool enabled( const std::vector< std::string>& memberships) const;

            //! extracts the current group configuration from the coordinator
            //! @returns a vector of all groups
            std::vector< model::domain::Group> config() const;

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( m_groups);
            );

         private:
            std::vector< Group> m_groups;
         };

      }
   }

} // casual