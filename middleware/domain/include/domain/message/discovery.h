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
         namespace content
         {
            struct Queue : common::Compare< Queue>
            {
               Queue() = default;
               inline Queue( std::string name, platform::size::type retries) : name{ std::move( name)}, retries{ retries} {}
               inline explicit Queue( std::string name) : name{ std::move( name)} {}

               std::string name;
               platform::size::type retries{};

               inline friend bool operator == ( const Queue& lhs, std::string_view rhs) { return lhs.name == rhs;}

               inline auto tie() const noexcept { return std::tie( name);}

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( name);
                  CASUAL_SERIALIZE( retries);
               )
            };

            using Service = common::message::service::concurrent::advertise::Service;
            
         } // content  
         


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
            std::vector< content::Service> services;
            std::vector< content::Queue> queues;
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

      namespace topology
      {
         namespace implicit
         {
            using base_update = common::message::basic_process< common::message::Type::domain_discovery_topology_implicit_update>;
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
            using base_update = common::message::basic_process< common::message::Type::domain_discovery_topology_direct_update>;
            struct Update : base_update
            {
               using base_update::base_update;

                //! What the _new connection_ is configured with, if any.
               discovery::request::Content configured;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_update::serialize( archive);
                  CASUAL_SERIALIZE( configured);
               )
            };

            using base_request = common::message::basic_message< common::message::Type::domain_discovery_topology_direct_explore>;

            //! sent from _discovery_ to discover/explore 'known' for "new connections".
            struct Explore : base_request
            {
               using base_request::base_request;
               discovery::request::Content content;
               
               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_request::serialize( archive);
                  CASUAL_SERIALIZE( content);
               )
            };
         } // direct
      
      } // topology

      namespace lookup
      {
         namespace request
         {
            enum struct Scope : std::uint16_t
            {
               internal,
               extended
            };

            constexpr std::string_view description( Scope value)
            {
               switch( value)
               {
                  case Scope::internal: return "internal";
                  case Scope::extended: return "extended";
               }
               return "<unknown>";
            }

         } // request

         using base_request = common::message::basic_request< common::message::Type::domain_discovery_lookup_request>;
         struct Request : base_request
         {
            using base_request::base_request;

            request::Scope scope{};
            //! wanted
            discovery::request::Content content;

            CASUAL_CONST_CORRECT_SERIALIZE(
               base_request::serialize( archive);
               CASUAL_SERIALIZE( scope);
               CASUAL_SERIALIZE( content);
            )
         };

         using base_reply = common::message::basic_reply< common::message::Type::domain_discovery_lookup_reply>;
         struct Reply : base_reply
         {
            using base_reply::base_reply;

            //! looked up/found and provided content
            discovery::reply::Content content;
            discovery::request::Content absent;

            CASUAL_CONST_CORRECT_SERIALIZE(
               base_reply::serialize( archive);
               CASUAL_SERIALIZE( content);
               CASUAL_SERIALIZE( absent);
            )
         };

      } // lookup

      namespace fetch::known
      {
         using Request = common::message::basic_request< common::message::Type::domain_discovery_fetch_known_request>;

         using base_reply = common::message::basic_request< common::message::Type::domain_discovery_fetch_known_reply>;
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
         
      } // fetch::known

      namespace api
      {
         namespace provider::registration
         {
            enum struct Ability : std::uint16_t
            {
               discover = 1,
               lookup = 2,
               fetch_known = 4,
               topology = 8,
            };

            consteval void casual_enum_as_flag( Ability);
            
            constexpr std::string_view description( Ability value)
            {
               switch( value)
               {
                  case Ability::discover: return "discover";
                  case Ability::lookup: return "lookup";
                  case Ability::fetch_known: return "fetch_known";
                  case Ability::topology: return "topology";
               }
               return "<unknown>";
            }
            
            using base_request = common::message::basic_request< common::message::Type::domain_discovery_api_provider_registration_request>;
            struct Request : base_request
            {
               using base_request::base_request;

               Ability abilities;

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

            using Content = discovery::reply::Content;

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
      struct type_traits< casual::domain::message::discovery::lookup::Request> : detail::type< casual::domain::message::discovery::lookup::Reply> {};

      template<>
      struct type_traits< casual::domain::message::discovery::fetch::known::Request> : detail::type< casual::domain::message::discovery::fetch::known::Reply> {};
      
   } // common::message::reverse
} // casual