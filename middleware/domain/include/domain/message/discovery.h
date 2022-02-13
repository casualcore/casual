//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/message/type.h"

#include "common/message/service.h"

#include <iosfwd>

namespace casual
{
   namespace domain::message::discovery
   {
      namespace request
      {
         enum struct Directive : std::uint16_t
         {
            local,
            forward
         };

         inline std::ostream& operator << ( std::ostream& out, Directive value)
         {
            switch( value)
            {
               case Directive::local: return out << "local";
               case Directive::forward: return out << "forward";
            }
            return out << "<unknown>";
         }

         struct Content
         {
            std::vector< std::string> services;
            std::vector< std::string> queues;

            inline Content& operator += ( Content rhs)
            {
               common::algorithm::append_unique( std::move( rhs.services), services);
               common::algorithm::append_unique( std::move( rhs.queues), queues);
               return *this;
            };

            inline explicit operator bool() const noexcept { return ! services.empty() || ! queues.empty();}

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( services);
               CASUAL_SERIALIZE( queues);
            )
         };

      } // request

      namespace reply
      {
         struct Queue
         {
            Queue() = default;
            inline Queue( std::string name) : name{ std::move( name)} {}

            std::string name;
            platform::size::type retries{};

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( name);
               CASUAL_SERIALIZE( retries);
            )
         };

         using Service = common::message::service::concurrent::advertise::Service;

         struct Content
         {
            std::vector< Service> services;
            std::vector< Queue> queues;

            inline explicit operator bool () const noexcept { return ! services.empty() || ! queues.empty();}

            inline Content& operator += ( Content other)
            {
               common::algorithm::append( std::move( other.services), services);
               common::algorithm::append( std::move( other.queues), queues);
               return *this;
            }

            inline friend Content operator + ( Content lhs, Content rhs) { return lhs += std::move( rhs);}

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( services);
               CASUAL_SERIALIZE( queues);
            )
         };
      } // reply


      using base_request = common::message::basic_request< common::message::Type::domain_discovery_request>;
      struct Request : base_request
      {
         using base_request::base_request;
         
         //! note: we might want to separate the directive to it's own message only, used from inbounds...
         //! but we roll with this _overhead_ for the time being...
         request::Directive directive = request::Directive::local;
         common::domain::Identity domain;
         discovery::request::Content content;

         CASUAL_CONST_CORRECT_SERIALIZE(
            base_request::serialize( archive);
            CASUAL_SERIALIZE( directive);
            CASUAL_SERIALIZE( domain);
            CASUAL_SERIALIZE( content);
         )
      };

      using base_reply = common::message::basic_reply< common::message::Type::domain_discovery_reply>;
      struct Reply : base_reply
      {
         using base_reply::base_reply;

         common::domain::Identity domain;
         discovery::reply::Content content;

         CASUAL_CONST_CORRECT_SERIALIZE(
            base_reply::serialize( archive);
            CASUAL_SERIALIZE( domain);
            CASUAL_SERIALIZE( content);
         )
      };

      namespace topology
      {
         using base_update = common::message::basic_message< common::message::Type::domain_discovery_topology_update>;
         struct Update : base_update
         {
            using base_update::base_update;

            //! domains that has seen/handled the message.
            std::vector< common::domain::Identity> domains;

            CASUAL_CONST_CORRECT_SERIALIZE(
               base_update::serialize( archive);
               CASUAL_SERIALIZE( domains);
            )
         };
      
      } // topology

      namespace needs
      {
         using base_request = common::message::basic_request< common::message::Type::domain_discovery_needs_request>;
         struct Request : base_request
         {
            using base_request::base_request;

            CASUAL_CONST_CORRECT_SERIALIZE(
               base_request::serialize( archive);
            )
         };


         using base_reply = common::message::basic_request< common::message::Type::domain_discovery_needs_reply>;
         using Content = discovery::request::Content;

         //! Contains what externals needs the requested has (advertised and _waiting lookups_)
         struct Reply : base_reply
         {
            using base_reply::base_reply;
            Content content;

            CASUAL_CONST_CORRECT_SERIALIZE(
               base_reply::serialize( archive);
               CASUAL_SERIALIZE( content);
            )
         };

      } // needs

      namespace api
      {
         namespace provider::registration
         {
            enum struct Ability : std::uint16_t
            {
               discover_internal = 1,
               discover_external = 2,
               needs = 4,
               topology = 8,
            };

            constexpr auto description( Ability value)
            {
               switch( value)
               {
                  case Ability::discover_internal: return std::string_view{ "discover_internal"};
                  case Ability::discover_external: return std::string_view{ "discover_external"};
                  case Ability::needs: return std::string_view{ "needs"};
                  case Ability::topology: return std::string_view{ "topology"};
               }
               return std::string_view{ "<unknown>"};
            }
            
            using base_request = common::message::basic_request< common::message::Type::domain_discovery_api_provider_registration_request>;
            struct Request : base_request
            {
               using base_request::base_request;

               common::Flags< registration::Ability> abilities;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_request::serialize( archive);
                  CASUAL_SERIALIZE( abilities);
               )
            };

            using Reply = common::message::basic_message< common::message::Type::domain_discovery_api_provider_registration_reply>;
            
         } // provider::registration
         
         using base_request = common::message::basic_request< common::message::Type::domain_discovery_api_request>;
         struct Request : base_request
         {
            using base_request::base_request;

            discovery::request::Content content;

            CASUAL_CONST_CORRECT_SERIALIZE(
               base_request::serialize( archive);
               CASUAL_SERIALIZE( content);
            )
         };


         using base_reply = common::message::basic_message< common::message::Type::domain_discovery_api_reply>;
         struct Reply : base_reply
         {
            using base_reply::base_reply;

            CASUAL_CONST_CORRECT_SERIALIZE(
               base_reply::serialize( archive);
            )
         };

         namespace rediscovery
         {
            using Request = common::message::basic_request< common::message::Type::domain_discovery_api_rediscovery_request>;

            using Content = discovery::request::Content;

            using base_reply = common::message::basic_message< common::message::Type::domain_discovery_api_rediscovery_reply>;
            struct Reply : base_reply
            {
               using base_reply::base_reply;

               Content content;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_reply::serialize( archive);
                  CASUAL_SERIALIZE( content);
               )
            };
            
         } // rediscovery

      } // api

   } // domain::message::discovery 

   namespace common::message::reverse
   {
      template<>
      struct type_traits< casual::domain::message::discovery::api::provider::registration::Request> : detail::type< casual::domain::message::discovery::api::provider::registration::Reply> {};

      template<>
      struct type_traits< casual::domain::message::discovery::api::Request> : detail::type< casual::domain::message::discovery::api::Reply> {};

      template<>
      struct type_traits< casual::domain::message::discovery::api::rediscovery::Request> : detail::type< casual::domain::message::discovery::api::rediscovery::Reply> {};


      template<>
      struct type_traits< casual::domain::message::discovery::Request> : detail::type< casual::domain::message::discovery::Reply> {};

      template<>
      struct type_traits< casual::domain::message::discovery::needs::Request> : detail::type< casual::domain::message::discovery::needs::Reply> {};
      
   } // common::message::reverse
} // casual