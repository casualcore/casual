//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MESSAGE_GATEWAY_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MESSAGE_GATEWAY_H_


#include "common/message/type.h"

#include "common/process.h"

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
            namespace service
            {
               namespace discover
               {
                  struct Request : basic_message< Type::gateway_service_discover_request>
                  {
                     common::process::Handle process;
                     std::vector< std::string> services;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_type::marshal( archive);
                        archive & process;
                        archive & services;
                     })
                  };

                  struct Reply : basic_message< Type::gateway_service_discover_request>
                  {
                     common::process::Handle process;
                     std::vector< std::string> services;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_type::marshal( archive);
                        archive & process;
                        archive & services;
                     })
                  };

               } // discover

            } // service

         } // gateway
      } // message
   } // common
} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MESSAGE_GATEWAY_H_
