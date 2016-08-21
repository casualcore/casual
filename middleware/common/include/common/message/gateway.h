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


               namespace service
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

                  } // advertise


                  struct Advertise : basic_message< Type::gateway_service_advertise>
                  {
                     common::process::Handle process;
                     common::domain::Identity domain;
                     std::size_t order = 0;
                     std::vector< advertise::Service> services;


                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_type::marshal( archive);
                        archive & process;
                        archive & services;
                     })

                     friend std::ostream& operator << ( std::ostream& out, const Advertise& message);
                  };

                  struct Unadvertise : basic_message< Type::gateway_service_unadvertise>
                  {
                     common::process::Handle process;
                     std::vector< std::string> services;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_type::marshal( archive);
                        archive & process;
                        archive & services;
                     })

                     friend std::ostream& operator << ( std::ostream& out, const Unadvertise& message);
                  };

               } // service

               namespace discover
               {
                  struct Request : basic_message< Type::gateway_domain_discover_request>
                  {
                     common::process::Handle process;
                     common::domain::Identity domain;
                     std::vector< std::string> services;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_type::marshal( archive);
                        archive & process;
                        archive & domain;
                        archive & services;
                     })

                     friend std::ostream& operator << ( std::ostream& out, const Request& value);
                  };

                  struct Reply : basic_message< Type::gateway_domain_discover_reply>
                  {
                     using Service = service::advertise::Service;

                     common::process::Handle process;
                     common::domain::Identity domain;
                     std::vector< Service> services;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_type::marshal( archive);
                        archive & process;
                        archive & domain;
                        archive & services;
                     })

                     friend std::ostream& operator << ( std::ostream& out, const Reply& value);
                  };

                  namespace automatic
                  {
                     struct Request : basic_message< Type::gateway_domain_automatic_discover_request>
                     {
                        common::process::Handle process;
                        common::domain::Identity domain;
                        std::vector< std::string> services;

                        CASUAL_CONST_CORRECT_MARSHAL(
                        {
                           base_type::marshal( archive);
                           archive & process;
                           archive & domain;
                           archive & services;
                        })

                        friend std::ostream& operator << ( std::ostream& out, const Request& value);
                     };

                     struct Reply : basic_message< Type::gateway_domain_automatic_discover_reply>
                     {
                        std::vector< discover::Reply> replies;

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
            struct type_traits< gateway::domain::discover::Request> : detail::type< gateway::domain::discover::Reply> {};

         } // reverse

      } // message
   } // common
} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MESSAGE_GATEWAY_H_
