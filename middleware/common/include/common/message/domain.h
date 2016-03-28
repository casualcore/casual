//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MESSAGE_DOMAIN_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MESSAGE_DOMAIN_H_

#include "common/message/type.h"
#include "common/domain.h"

namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace domain
         {


            namespace discover
            {
               struct Request : common::message::basic_message< common::message::Type::domain_discover_request>
               {
                  common::domain::Identity remote;


                  //!
                  //!
                  //!
                  std::vector< std::string> services;


                  CASUAL_CONST_CORRECT_MARSHAL({
                     base_type::marshal( archive);
                     archive & remote;
                     archive & services;
                  })
               };

               struct Reply : common::message::basic_message< common::message::Type::domain_discover_reply>
               {
                  common::domain::Identity remote;


                  CASUAL_CONST_CORRECT_MARSHAL({
                     base_type::marshal( archive);
                     archive & remote;
                  })
               };


            } // discover


         } // domain

      } // message
   } // common

} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MESSAGE_DOMAIN_H_
