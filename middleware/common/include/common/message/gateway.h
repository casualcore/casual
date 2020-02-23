//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "common/message/service.h"
#include "common/message/queue.h"

#include "common/process.h"
#include "common/domain.h"

#include <vector>
#include <string>

namespace casual
{

   namespace common
   {
      namespace message
      {
         namespace gateway
         {
            namespace domain
            {
               using size_type = platform::size::type;

               namespace protocol
               {
                  enum class Version : size_type
                  {
                     invalid = 0,
                     version_1 = 1000,

                     // make sure this 'points' to the latest version
                     latest = version_1
                  };
               } // protocol

               namespace connect
               {
                  using base_request = basic_message< Type::gateway_domain_connect_request>;
                  struct Request : base_request
                  {
                     using base_request::base_request;

                     common::domain::Identity domain;
                     std::vector< protocol::Version> versions;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_request::serialize( archive);
                        CASUAL_SERIALIZE( domain);
                        CASUAL_SERIALIZE( versions);
                     })
                  };

                  using base_reply = basic_message< Type::gateway_domain_connect_reply>;
                  struct Reply : base_reply
                  {
                     using base_reply::base_reply;

                     common::domain::Identity domain;
                     protocol::Version version = protocol::Version::invalid;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_reply::serialize( archive);
                        CASUAL_SERIALIZE( domain);
                        CASUAL_SERIALIZE( version);
                     })
                  };
               } // connect

               namespace discover
               {
                  //! Request from another domain to the local gateway, that's then
                  //! 'forwarded' to broker and possible casual-queue to revel stuff about
                  //! this domain.
                  //!
                  //! other domain -> inbound-connection -> gateway ---> casual-broker
                  //!                                               [ \-> casual-queue ]
                  using base_request = basic_request< Type::gateway_domain_discover_request>;
                  struct Request :base_request
                  {
                     using base_request::base_request;

                     common::domain::Identity domain;
                     std::vector< std::string> services;
                     std::vector< std::string> queues;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_request::serialize( archive);
                        CASUAL_SERIALIZE( domain);
                        CASUAL_SERIALIZE( services);
                        CASUAL_SERIALIZE( queues);
                     })

                  };

                  //! Reply from a domain
                  //!    [casual-queue -\ ]
                  //!    casual-broker ----> gateway -> inbound-connection -> other domain
                  using base_reply = basic_request< Type::gateway_domain_discover_reply>;
                  struct Reply : base_reply
                  {
                     using Service = service::concurrent::advertise::Service;
                     using Queue = queue::concurrent::advertise::Queue;

                     common::domain::Identity domain;
                     std::vector< Service> services;
                     std::vector< Queue> queues;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_reply::serialize( archive);
                        CASUAL_SERIALIZE( domain);
                        CASUAL_SERIALIZE( services);
                        CASUAL_SERIALIZE( queues);
                     })
                  };

                  namespace accumulated
                  {
                     //! Reply from the gateway with accumulated replies from other domains
                     //!
                     //!                   requester  <-- gateway <--- outbound connection -> domain 1
                     //!                                            \- outbound connection -> domain 2
                     //!                                               ...
                     //!                                             |- outbound connection -> domain N
                     using base_reply = basic_message< Type::gateway_domain_discover_accumulated_reply>;
                     struct Reply : base_reply
                     {
                        std::vector< discover::Reply> replies;

                        CASUAL_CONST_CORRECT_SERIALIZE(
                        {
                           base_reply::serialize( archive);
                           CASUAL_SERIALIZE( replies);
                        })
                     };
                  } // accumulated
               } // discover
            } // domain
         } // gateway

         namespace reverse
         {
            template<>
            struct type_traits< gateway::domain::discover::Request> : detail::type< gateway::domain::discover::Reply> {};

            template<>
            struct type_traits< gateway::domain::connect::Request> : detail::type< gateway::domain::connect::Reply> {};

         } // reverse

      } // message
   } // common
} // casual


