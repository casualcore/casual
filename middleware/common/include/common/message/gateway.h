//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MESSAGE_GATEWAY_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MESSAGE_GATEWAY_H_


#include "common/message/service.h"

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

               namespace advertise
               {
                  //!
                  //! Represent service information in a 'remote advertise context'
                  //!
                  struct Service : message::Service
                  {
                     Service() = default;
                     Service( std::string name,
                           std::uint64_t type = 0,
                           common::service::transaction::Type transaction = common::service::transaction::Type::automatic,
                           std::size_t hops = 0)
                      : message::Service{ std::move( name), type, transaction}, hops{ hops} {}

                     std::size_t hops = 0;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        message::Service::marshal( archive);
                        archive & hops;
                     })

                     friend std::ostream& operator << ( std::ostream& out, const Service& message);
                  };

                  struct Queue
                  {
                     Queue() = default;
                     Queue( std::string name) : name{ std::move( name)} {}

                     std::string name;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        archive & name;
                     })

                     friend std::ostream& operator << ( std::ostream& out, const Queue& message);
                  };

               } // advertise


               struct Advertise : basic_message< Type::gateway_domain_advertise>
               {
                  enum class Directive : char
                  {
                     add,
                     remove,
                     replace
                  };

                  Directive directive;

                  common::process::Handle process;
                  common::domain::Identity domain;
                  std::size_t order = 0;
                  std::vector< advertise::Service> services;
                  std::vector< advertise::Queue> queues;


                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     base_type::marshal( archive);
                     archive & directive;
                     archive & process;
                     archive & domain;
                     archive & order;
                     archive & services;
                     archive & queues;
                  })

                  friend std::ostream& operator << ( std::ostream& out, Directive message);
                  friend std::ostream& operator << ( std::ostream& out, const Advertise& message);
               };



               namespace discover
               {
                  namespace internal
                  {
                     //!
                     //! Request from another domain to the local gateway, that's then
                     //! 'forwarded' to broker and possible casual-queue to revel stuff about
                     //! this domain.
                     //!
                     //! other domain -> inbound-connection -> gateway ---> casual-broker
                     //!                                               [ \-> casual-gueue ]
                     //!
                     struct Request : basic_message< Type::gateway_domain_discover_request>
                     {
                        common::process::Handle process;
                        common::domain::Identity domain;
                        std::vector< std::string> services;
                        std::vector< std::string> queues;

                        CASUAL_CONST_CORRECT_MARSHAL(
                        {
                           base_type::marshal( archive);
                           archive & process;
                           archive & domain;
                           archive & services;
                           archive & queues;
                        })

                        friend std::ostream& operator << ( std::ostream& out, const Request& value);
                     };

                     //!
                     //! Reply from a domain
                     //!    [casual-queue -\ ]
                     //!    casual-broker ----> gateway -> inbound-connection -> other domain
                     //!
                     struct Reply : basic_message< Type::gateway_domain_discover_reply>
                     {
                        using Service = domain::advertise::Service;
                        using Queue = domain::advertise::Queue;

                        common::process::Handle process;
                        common::domain::Identity domain;
                        std::vector< Service> services;
                        std::vector< Queue> queues;

                        CASUAL_CONST_CORRECT_MARSHAL(
                        {
                           base_type::marshal( archive);
                           archive & process;
                           archive & domain;
                           archive & services;
                        })

                        friend std::ostream& operator << ( std::ostream& out, const Reply& value);
                     };

                  } // internal

                  namespace external
                  {
                     //!
                     //! Request from within a domain to the gateway to discover stuff about
                     //! other domains
                     //!
                     //! casual-broker | casual-queue -> gateway ---> outbound connection -> domain 1
                     //!                                          |-> outbound connection -> domain 2
                     //!                                             ...
                     //!                                          |-> outbound connection -> domain N
                     //!
                     struct Request : basic_message< Type::gateway_domain_automatic_discover_request>
                     {
                        common::process::Handle process;
                        common::domain::Identity domain;
                        std::vector< std::string> services;
                        std::vector< std::string> queues;

                        CASUAL_CONST_CORRECT_MARSHAL(
                        {
                           base_type::marshal( archive);
                           archive & process;
                           archive & domain;
                           archive & services;
                        })

                        friend std::ostream& operator << ( std::ostream& out, const Request& value);
                     };

                     //!
                     //! Reply from the gateway with accumulated replies from other domains
                     //!
                     //!
                     struct Reply : basic_message< Type::gateway_domain_automatic_discover_reply>
                     {
                        std::vector< discover::internal::Reply> replies;

                        CASUAL_CONST_CORRECT_MARSHAL(
                        {
                           base_type::marshal( archive);
                           archive & replies;
                        })

                        friend std::ostream& operator << ( std::ostream& out, const Reply& value);
                     };
                  } // automatic

               } // discover

            } // domain

         } // gateway

         namespace reverse
         {
            template<>
            struct type_traits< gateway::domain::discover::internal::Request> : detail::type< gateway::domain::discover::internal::Reply> {};

         } // reverse

      } // message
   } // common
} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MESSAGE_GATEWAY_H_
