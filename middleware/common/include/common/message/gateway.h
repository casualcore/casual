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
               using size_type = platform::size::type;

               namespace protocol
               {
                  enum class Version : size_type
                  {
                     invalid = 0,
                     version_1 = 1000,
                  };
               } // protocol


               namespace advertise
               {
                  //!
                  //! Represent service information in a 'remote advertise context'
                  //!
                  struct Service : message::Service
                  {
                     Service() = default;
                     Service( std::string name,
                           std::string category = {},
                           common::service::transaction::Type transaction = common::service::transaction::Type::automatic,
                           size_type hops = 0)
                      : message::Service{ std::move( name), std::move( category), transaction}, hops{ hops} {}

                      Service( std::function<void(Service&)> foreign) { foreign( *this);}

                     size_type hops = 0;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        message::Service::marshal( archive);
                        archive & hops;
                     })

                     friend std::ostream& operator << ( std::ostream& out, const Service& message);
                  };
                  static_assert( traits::is_movable< Service>::value, "not movable");

                  struct Queue
                  {
                     Queue() = default;

                     Queue( std::string name, size_type retries) : name{ std::move( name)}, retries{ retries} {}
                     Queue( std::string name) : name{ std::move( name)} {}

                     Queue( std::function< void(Queue&)> foreign) { foreign( *this);}

                     std::string name;
                     size_type retries = 0;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        archive & name;
                        archive & retries;
                     })

                     friend std::ostream& operator << ( std::ostream& out, const Queue& message);
                  };
                  static_assert( traits::is_movable< Queue>::value, "not movable");

               } // advertise


               struct Advertise : basic_message< Type::gateway_domain_advertise>
               {
                  enum class Directive : char
                  {
                     add,
                     remove,
                     replace
                  };

                  Directive directive = Directive::add;

                  common::process::Handle process;
                  common::domain::Identity domain;
                  platform::size::type order = 0;
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
               static_assert( traits::is_movable< Advertise>::value, "not movable");


               namespace connect
               {

                  struct Request : basic_message< Type::gateway_domain_connect_request>
                  {
                     common::domain::Identity domain;
                     std::vector< protocol::Version> versions;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_type::marshal( archive);
                        archive & domain;
                        archive & versions;
                     })

                     friend std::ostream& operator << ( std::ostream& out, const Request& value);
                  };

                  struct Reply : basic_message< Type::gateway_domain_connect_reply>
                  {
                     common::domain::Identity domain;
                     protocol::Version version;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_type::marshal( archive);
                        archive & domain;
                        archive & version;
                     })

                     friend std::ostream& operator << ( std::ostream& out, const Reply& value);
                  };
               } // connect

               namespace discover
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
                  static_assert( traits::is_movable< Request>::value, "not movable");

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
                        archive & queues;
                     })

                     friend std::ostream& operator << ( std::ostream& out, const Reply& value);
                  };
                  static_assert( traits::is_movable< Reply>::value, "not movable");


                  namespace accumulated
                  {

                     //!
                     //! Reply from the gateway with accumulated replies from other domains
                     //!
                     //!                   requester  <-- gateway <--- outbound connection -> domain 1
                     //!                                            \- outbound connection -> domain 2
                     //!                                               ...
                     //!                                             |- outbound connection -> domain N
                     //!
                     struct Reply : basic_message< Type::gateway_domain_discover_accumulated_reply>
                     {
                        std::vector< discover::Reply> replies;

                        CASUAL_CONST_CORRECT_MARSHAL(
                        {
                           base_type::marshal( archive);
                           archive & replies;
                        })

                        friend std::ostream& operator << ( std::ostream& out, const Reply& value);
                     };
                     static_assert( traits::is_movable< Reply>::value, "not movable");
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

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MESSAGE_GATEWAY_H_
