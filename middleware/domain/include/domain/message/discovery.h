//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/message/type.h"

#include "common/message/service.h"
#include "common/algorithm/sorted.h"

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

         constexpr std::string_view description( Directive value) noexcept
         {
            switch( value)
            {
               case Directive::local: return "local";
               case Directive::forward: return "forward";
            }
            return "<unknown>";
         }

         struct Content
         {
            inline Content& operator += ( Content other)
            {
               common::algorithm::sorted::append_unique( std::move( other.services), services);
               common::algorithm::sorted::append_unique( std::move( other.queues), queues);
               return *this;
            };

            inline friend Content operator + ( Content lhs, Content rhs) { lhs += std::move( rhs); return lhs;}


            //! logical ordered unique set. 
            //! @attention it's the _mutater_ responsibility to restore the invariant.
            //! @{
            std::vector< std::string> services;
            std::vector< std::string> queues;
            //! @}

            inline explicit operator bool() const noexcept { return ! services.empty() || ! queues.empty();}

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( services);
               CASUAL_SERIALIZE( queues);
            )

         };

      } // request

      namespace reply
      {
         struct Queue : common::Compare< Queue>
         {
            Queue() = default;
            inline Queue( std::string name) : name{ std::move( name)} {}

            std::string name;
            platform::size::type retries{};

            inline auto tie() const noexcept { return std::tie( name);}

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( name);
               CASUAL_SERIALIZE( retries);
            )
         };

         using Service = common::message::service::concurrent::advertise::Service;

         struct Content
         {
            inline explicit operator bool () const noexcept { return ! services.empty() || ! queues.empty();}

            inline Content& operator += ( Content other)
            {
               common::algorithm::sorted::append_unique( std::move( other.services), services);
               common::algorithm::sorted::append_unique( std::move( other.queues), queues);
               return *this;
            }

            inline friend Content operator + ( Content lhs, Content rhs) { return lhs += std::move( rhs);}

            //! logical ordered unique set. 
            //! @attention it's the _mutater_ responsibility to restore the invariant.
            //! @{
            std::vector< Service> services;
            std::vector< Queue> queues;
            //! @}

         
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

      namespace internal
      {
         using base_request = common::message::basic_request< common::message::Type::domain_discovery_internal_request>;
         struct Request : base_request
         {
            using base_request::base_request;
            
            discovery::request::Content content;

            CASUAL_CONST_CORRECT_SERIALIZE(
               base_request::serialize( archive);
               CASUAL_SERIALIZE( content);
            )
         };

         namespace reply::service  
         {
            struct Route : common::Compare< Route>
            {
               Route() = default;
               Route( std::string name, std::string origin) : name{ std::move( name)}, origin{ std::move( origin)} {}

               std::string name;
               std::string origin;

               inline auto tie() const noexcept { return std::tie( name);}


               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( name);
                  CASUAL_SERIALIZE( origin);
               )
            };

            struct Routes
            {
               //! only services, for now...
               std::vector< Route> services;

               inline Routes& operator += ( Routes other)
               {
                  common::algorithm::sorted::append_unique( common::algorithm::sort( std::move( other.services)), services);
                  return *this;
               }

               explicit operator bool() const noexcept { return ! services.empty();}

               auto find_name( const std::string& value) const noexcept 
               {
                  return common::algorithm::find_if( services, [ &value]( const Route& route){ return route.name == value;});
               }

               auto find_origin( const std::string& value) const noexcept 
               {
                  return common::algorithm::find_if( services, [ &value]( const Route& route){ return route.origin == value;});
               }


               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( services);
               )
            };
         } // reply::service 

         using base_reply = common::message::basic_reply< common::message::Type::domain_discovery_internal_reply>;
         struct Reply : base_reply
         {
            using base_reply::base_reply;

            discovery::reply::Content content;

            //! holds mapping between requested services that are routes, to the real (origin) name
            reply::service::Routes routes;

            CASUAL_CONST_CORRECT_SERIALIZE(
               base_reply::serialize( archive);
               CASUAL_SERIALIZE( content);
               CASUAL_SERIALIZE( routes);
            )
         };

      } // internal

      namespace topology
      {
         namespace implicit
         {
            using base_update = common::message::basic_message< common::message::Type::domain_discovery_topology_implicit_update>;
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
         } // implicit

         namespace direct
         {
            using base_update = common::message::basic_message< common::message::Type::domain_discovery_topology_direct_update>;
            struct Update : base_update
            {
               using base_update::base_update;

               // filled with the configured "resources" (that outbound could have)
               discovery::request::Content content;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_update::serialize( archive);
                  CASUAL_SERIALIZE( content);
               )
            };
         } // direct
      
      } // topology

      namespace needs
      {
         using Request = common::message::basic_request< common::message::Type::domain_discovery_needs_request>;

         using base_reply = common::message::basic_request< common::message::Type::domain_discovery_needs_reply>;
         using Content = discovery::request::Content;

         //! contains ONLY _waiting lookups_. The needs not yet fullfilled. 
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

      namespace known
      {
         using Request = common::message::basic_request< common::message::Type::domain_discovery_known_request>;

         using base_reply = common::message::basic_request< common::message::Type::domain_discovery_known_reply>;
         using Content = discovery::request::Content;

         //! Contains all "resources" that each provider knows about
         struct Reply : base_reply
         {
            using base_reply::base_reply;
            Content content;

            CASUAL_CONST_CORRECT_SERIALIZE(
               base_reply::serialize( archive);
               CASUAL_SERIALIZE( content);
            )
         };
         
      } // known

      namespace api
      {
         namespace provider::registration
         {
            enum struct Ability : std::uint16_t
            {
               discover_internal = 1,
               discover_external = 2,
               needs = 4,
               known = 8,
               topology = 16,
            };

            using Abilities = common::Flags< registration::Ability>;

            constexpr std::string_view description( Ability value)
            {
               switch( value)
               {
                  case Ability::discover_internal: return "discover_internal";
                  case Ability::discover_external: return "discover_external";
                  case Ability::needs: return "needs";
                  case Ability::known: return "known";
                  case Ability::topology: return "topology";
               }
               return "<unknown>";
            }
            
            using base_request = common::message::basic_request< common::message::Type::domain_discovery_api_provider_registration_request>;
            struct Request : base_request
            {
               using base_request::base_request;

               Abilities abilities;

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

            discovery::reply::Content content;

            CASUAL_CONST_CORRECT_SERIALIZE(
               base_reply::serialize( archive);
               CASUAL_SERIALIZE( content);
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
      struct type_traits< casual::domain::message::discovery::internal::Request> : detail::type< casual::domain::message::discovery::internal::Reply> {};

      template<>
      struct type_traits< casual::domain::message::discovery::needs::Request> : detail::type< casual::domain::message::discovery::needs::Reply> {};

      template<>
      struct type_traits< casual::domain::message::discovery::known::Request> : detail::type< casual::domain::message::discovery::known::Reply> {};
      
   } // common::message::reverse
} // casual