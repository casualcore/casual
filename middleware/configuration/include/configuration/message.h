//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "configuration/model.h"

#include "common/message/type.h"

namespace casual
{
   namespace configuration
   {
      namespace message
      {

         template< typename Model, common::message::Type type>
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
         using Reply = basic_model< model::queue::Model, common::message::Type::configuration_reply>;

         namespace update
         {
            using Request = basic_model< model::gateway::Model, common::message::Type::configuration_update_request>;
            using Reply = common::message::basic_request< common::message::Type::configuration_update_reply>;
            
         } // update

         namespace put
         {
            using Request = basic_model< model::gateway::Model, common::message::Type::configuration_put_request>;
            using Reply = common::message::basic_request< common::message::Type::configuration_put_reply>;
            
         } // put

         
      } // message
   } // configuration

   namespace common
   {
      namespace message
      {
         namespace reverse
         {
            template<>
            struct type_traits< casual::configuration::message::Request> : detail::type< casual::configuration::message::Reply> {};

            template<>
            struct type_traits< casual::configuration::message::update::Request> : detail::type< casual::configuration::message::update::Reply> {};


         } // reverse
      } // message
   } // common
} // casual