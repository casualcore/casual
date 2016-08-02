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
                     std::vector< Service> services;
                     common::domain::Identity remote;
                     std::vector< std::string> address;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_type::marshal( archive);
                        archive & process;
                        archive & services;
                        archive & remote;
                        archive & address;
                     })

                     friend std::ostream& operator << ( std::ostream& out, const Reply& value);
                  };

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
