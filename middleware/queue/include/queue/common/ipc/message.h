//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/message/type.h"
#include "common/message/queue.h"

#include "configuration/model.h"

namespace casual
{
   namespace queue
   {
      namespace ipc
      {
         namespace message
         {
            namespace forward
            {
               namespace configuration
               {
                  using Request = common::message::basic_request< common::message::Type::queue_forward_configuration_request>;
                  
                  using base_reply = common::message::basic_message< common::message::Type::queue_forward_configuration_reply>;
                  struct Reply : base_reply
                  {
                     casual::configuration::model::queue::Model model;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                        base_reply::serialize( archive);
                        CASUAL_SERIALIZE( model);
                     )
                  };
               } // configuration

               namespace state
               {
                  using Request = common::message::basic_request< common::message::Type::queue_forward_state_request>;

                  using base_reply = common::message::basic_request< common::message::Type::queue_forward_state_reply>;
                  struct Reply : base_reply
                  {
                     using base_reply::base_reply;

                     struct Instances
                     {
                        platform::size::type configured = 0;
                        platform::size::type running = 0;

                        CASUAL_CONST_CORRECT_SERIALIZE(
                           CASUAL_SERIALIZE( configured);
                           CASUAL_SERIALIZE( running);
                        )
                     };

                     struct Metric
                     {
                        struct Count
                        {
                           platform::size::type count = 0;
                           platform::time::point::type last{};

                           CASUAL_CONST_CORRECT_SERIALIZE(
                              CASUAL_SERIALIZE( count);
                              CASUAL_SERIALIZE( last);
                           )
                        };

                        Count commit;
                        Count rollback;

                        CASUAL_CONST_CORRECT_SERIALIZE(
                           CASUAL_SERIALIZE( commit);
                           CASUAL_SERIALIZE( rollback);
                        )
                     };

                     struct Queue
                     {
                        struct Target
                        {
                           std::string queue;
                           platform::time::unit delay{};

                           CASUAL_CONST_CORRECT_SERIALIZE(
                              CASUAL_SERIALIZE( queue);
                              CASUAL_SERIALIZE( delay);
                           )
                        };

                        std::string alias;
                        std::string source;
                        Target target;
                        Instances instances;
                        Metric metric;
                        std::string note;

                        CASUAL_CONST_CORRECT_SERIALIZE(
                           CASUAL_SERIALIZE( alias);
                           CASUAL_SERIALIZE( source);
                           CASUAL_SERIALIZE( target);
                           CASUAL_SERIALIZE( instances);
                           CASUAL_SERIALIZE( metric);
                           CASUAL_SERIALIZE( note);
                        )
                     };

                     struct Service
                     {
                        struct Target
                        {
                           std::string service;

                           CASUAL_CONST_CORRECT_SERIALIZE(
                              CASUAL_SERIALIZE( service);
                           )
                        };

                        using Reply = Queue::Target;
                        
                        std::string alias;
                        std::string source;
                        Target target;
                        Instances instances;
                        std::optional< Reply> reply;
                        Metric metric;
                        std::string note;

                        CASUAL_CONST_CORRECT_SERIALIZE(
                           CASUAL_SERIALIZE( alias);
                           CASUAL_SERIALIZE( source);
                           CASUAL_SERIALIZE( target);
                           CASUAL_SERIALIZE( instances);
                           CASUAL_SERIALIZE( reply);
                           CASUAL_SERIALIZE( metric);
                           CASUAL_SERIALIZE( note);
                        )
                     };

                     std::vector< Service> services;
                     std::vector< Queue> queues;

                     inline friend Reply operator + ( Reply lhs, Reply rhs)
                     {
                        common::algorithm::move( rhs.services, lhs.services);
                        common::algorithm::move( rhs.queues, lhs.queues);

                        return lhs;
                     }

                     CASUAL_CONST_CORRECT_SERIALIZE(
                        base_reply::serialize( archive);
                        CASUAL_SERIALIZE( services);
                        CASUAL_SERIALIZE( queues);
                     )
                  };

               } // state 

            } // forward

            namespace group
            {
               namespace connect
               {
                  using Request = common::message::basic_request< common::message::Type::queue_group_connect_request>;

                  using base_reply = common::message::basic_message< common::message::Type::queue_group_connect_reply>;
                  struct Reply : base_reply
                  {
                     using base_reply::base_reply;

                     using Queue = common::message::queue::Queue;

                     std::string name;
                     std::vector< Queue> queues;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_reply::serialize( archive);
                        CASUAL_SERIALIZE( name);
                        CASUAL_SERIALIZE( queues);
                     })
                  };
               } // connect
            } // group

         } // message
      } // ipc
   } // queue
   
   namespace common
   {
      namespace message
      {
         namespace reverse
         {

            template<>
            struct type_traits< casual::queue::ipc::message::forward::configuration::Request> : detail::type< casual::queue::ipc::message::forward::configuration::Reply> {};

            template<>
            struct type_traits< casual::queue::ipc::message::forward::state::Request> : detail::type< casual::queue::ipc::message::forward::state::Reply> {};

            template<>
            struct type_traits< casual::queue::ipc::message::group::connect::Request> : detail::type< casual::queue::ipc::message::group::connect::Reply> {};
         

         } // reverse

      } // message
   } // common
} // casual