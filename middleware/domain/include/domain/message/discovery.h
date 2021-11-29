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

      namespace internal::registration
      {
         using Request = common::message::basic_request< common::message::Type::domain_discovery_internal_registration_request>;
         using Reply = common::message::basic_message< common::message::Type::domain_discovery_internal_registration_reply>;
      } // internal::registration

      namespace request
      {
         enum struct Directive : short
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

            inline friend Content operator + ( Content lhs, Content rhs)
            {
               common::algorithm::append( std::move( rhs.queues), lhs.queues);
               common::algorithm::append( std::move( rhs.services), lhs.services);
               return lhs;
            }

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( services);
               CASUAL_SERIALIZE( queues);
            )
         };
      }


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


      namespace external
      {
         namespace registration
         {
            using base_request = common::message::basic_request< common::message::Type::domain_discovery_external_registration_request>;
            struct Request : base_request
            {
               using base_request::base_request;

               enum struct Directive
               {
                  regular,
                  rediscovery
               };

               Directive directive{};

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_request::serialize( archive);
                  CASUAL_SERIALIZE( directive);
               )
            };

            using Reply = common::message::basic_message< common::message::Type::domain_discovery_external_registration_reply>;
         } // registration



         using base_request = common::message::basic_request< common::message::Type::domain_discovery_external_request>;
         struct Request : base_request
         {
            using base_request::base_request;

            discovery::request::Content content;

            CASUAL_CONST_CORRECT_SERIALIZE(
               base_request::serialize( archive);
               CASUAL_SERIALIZE( content);
            )
         };


         using base_reply = common::message::basic_message< common::message::Type::domain_discovery_external_reply>;
         struct Reply : base_reply
         {
            using base_reply::base_reply;

            CASUAL_CONST_CORRECT_SERIALIZE(
               base_reply::serialize( archive);
            )
         };

         namespace advertised
         {
            using Request = common::message::basic_request< common::message::Type::domain_discovery_external_advertised_request>;

            using base_reply = common::message::basic_request< common::message::Type::domain_discovery_external_advertised_reply>;

            using Content = discovery::request::Content;

            //! Contains what externals has advertised
            struct Reply : base_reply
            {
               using base_reply::base_reply;

               Content content;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_reply::serialize( archive);
                  CASUAL_SERIALIZE( content);
               )
            };

            
         } // advertised
         
      } // external

      namespace rediscovery
      {
         using Request = common::message::basic_request< common::message::Type::domain_discovery_external_rediscovery_request>;
         using Reply = common::message::basic_message< common::message::Type::domain_discovery_external_rediscovery_reply>;
      } // rediscovery

   } // domain::message::discovery 

   namespace common::message::reverse
   {
      template<>
      struct type_traits< casual::domain::message::discovery::internal::registration::Request> : detail::type< casual::domain::message::discovery::internal::registration::Reply> {};

      template<>
      struct type_traits< casual::domain::message::discovery::external::registration::Request> : detail::type< casual::domain::message::discovery::external::registration::Reply> {};

      template<>
      struct type_traits< casual::domain::message::discovery::Request> : detail::type< casual::domain::message::discovery::Reply> {};

      template<>
      struct type_traits< casual::domain::message::discovery::external::Request> : detail::type< casual::domain::message::discovery::external::Reply> {};

      template<>
      struct type_traits< casual::domain::message::discovery::external::advertised::Request> : detail::type< casual::domain::message::discovery::external::advertised::Reply> {};

      template<>
      struct type_traits< casual::domain::message::discovery::rediscovery::Request> : detail::type< casual::domain::message::discovery::rediscovery::Reply> {};
      
   } // common::message::reverse
} // casual