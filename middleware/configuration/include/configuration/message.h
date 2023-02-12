//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "configuration/model.h"

#include "common/message/type.h"
#include "common/flag.h"

namespace casual
{
   namespace configuration::message
   {

      template< common::message::Type type>
      struct basic_model : common::message::basic_request< type>
      {
         using base_type = common::message::basic_request< type>;
         using base_type::base_type;

         configuration::Model model;

         CASUAL_CONST_CORRECT_SERIALIZE(
            base_type::serialize( archive);
            CASUAL_SERIALIZE( model);
         )
      };

      using Request = common::message::basic_request< common::message::Type::configuration_request>;
      using Reply = basic_model< common::message::Type::configuration_reply>;

      namespace stakeholder::registration
      {
         enum struct Ability : std::uint16_t
         {
            supply = 1,
            runtime_update = 2,
         };

         constexpr std::string_view description( Ability value)
         {
            switch( value)
            {
               case Ability::supply: return "supply";
               case Ability::runtime_update: return "runtime_update";
            }
            return "<unknown>";
         }

         using Contract = common::Flags< registration::Ability>;

         using base_request = common::message::basic_request< common::message::Type::configuration_stakeholder_registration_request>;
         struct Request : base_request
         {
            using base_request::base_request;

            Contract contract;

            CASUAL_CONST_CORRECT_SERIALIZE(
               base_request::serialize( archive);
               CASUAL_SERIALIZE( contract);
            )
         };

         using Reply = common::message::basic_reply< common::message::Type::configuration_stakeholder_registration_reply>;

      } // stakeholder::registration

      namespace update
      {
         using Request = basic_model< common::message::Type::configuration_update_request>;
         using Reply = common::message::basic_request< common::message::Type::configuration_update_reply>;
         
      } // update

      namespace put
      {
         using Request = basic_model< common::message::Type::configuration_put_request>;
         using Reply = common::message::basic_request< common::message::Type::configuration_put_reply>;
         
      } // put

   } // configuration::message

   namespace common::message::reverse
   {
      template<>
      struct type_traits< casual::configuration::message::Request> : detail::type< casual::configuration::message::Reply> {};

      template<>
      struct type_traits< casual::configuration::message::stakeholder::registration::Request> : detail::type< casual::configuration::message::stakeholder::registration::Reply> {};

      template<>
      struct type_traits< casual::configuration::message::update::Request> : detail::type< casual::configuration::message::update::Reply> {};

   } // common::message::reverse
} // casual