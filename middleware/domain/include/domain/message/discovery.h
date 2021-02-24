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

      namespace inbound
      {
         using Registration = common::message::basic_request< common::message::Type::domain_discovery_inbound_registration>;
      }

      namespace outbound
      {
         
      }


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


      namespace outbound
      {
         using base_registration = common::message::basic_request< common::message::Type::domain_discovery_outbound_registration>;
         struct Registration : base_registration
         {
            using base_registration::base_registration;

            enum struct Directive
            {
               regular,
               rediscovery
            };

            Directive directive{};

            CASUAL_CONST_CORRECT_SERIALIZE(
               base_registration::serialize( archive);
               CASUAL_SERIALIZE( directive);
            )
         };


         using base_request = common::message::basic_request< common::message::Type::domain_discovery_outbound_request>;
         struct Request : base_request
         {
            using base_request::base_request;

            discovery::request::Content content;

            CASUAL_CONST_CORRECT_SERIALIZE(
               base_request::serialize( archive);
               CASUAL_SERIALIZE( content);
            )
         };

         namespace reply
         {
            struct Outbound
            {
               common::process::Handle process;
               discovery::reply::Content content;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( process);
                  CASUAL_SERIALIZE( content);
               )
            };
         }

         using base_reply = common::message::basic_message< common::message::Type::domain_discovery_outbound_reply>;
         struct Reply : base_reply
         {
            using base_reply::base_reply;

            //std::vector< reply::Outbound> outbounds;

            CASUAL_CONST_CORRECT_SERIALIZE(
               base_reply::serialize( archive);
               //CASUAL_SERIALIZE( outbounds);
            )
         };
         
      } // outbound

      namespace rediscovery
      {
         using Request = common::message::basic_request< common::message::Type::domain_discovery_rediscovery_request>;
         using Reply = common::message::basic_message< common::message::Type::domain_discovery_rediscovery_reply>;
      } // rediscovery

   } // domain::message::discovery 

   namespace common::message::reverse
   {
      template<>
      struct type_traits< casual::domain::message::discovery::Request> : detail::type< casual::domain::message::discovery::Reply> {};

      template<>
      struct type_traits< casual::domain::message::discovery::outbound::Request> : detail::type< casual::domain::message::discovery::outbound::Reply> {};

      template<>
      struct type_traits< casual::domain::message::discovery::rediscovery::Request> : detail::type< casual::domain::message::discovery::rediscovery::Reply> {};
      
   } // common::message::reverse
} // casual