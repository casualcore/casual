//!
//! server.h
//!
//! Created on: Jun 14, 2014
//!     Author: Lazan
//!

#ifndef COMMON_MESSAGE_SERVER_H_
#define COMMON_MESSAGE_SERVER_H_


#include "common/message/type.h"

#include "common/uuid.h"

namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace server
         {
            namespace ping
            {

               struct Request : basic_id< cServerPingRequest>
               {

               };

               struct Reply : basic_id< cServerPingReply>
               {
                  using base_type = basic_id< cServerPingReply>;

                  Uuid uuid;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     base_type::marshal( archive);
                     archive & uuid;
                  })

               };

            } // ping

            namespace connect
            {
               struct Request : public basic_connect< cServerConnectRequest>
               {
                  std::vector< Service> services;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                        basic_connect< cServerConnectRequest>::marshal( archive);
                        archive & services;
                  })
               };


               //!
               //! Sent from the broker with "start-up-information" for a server
               //!
               struct Reply : basic_message< cServerConnectReply>
               {

                  Reply() = default;
                  Reply( Reply&&) = default;
                  Reply& operator = ( Reply&&) = default;

                  CASUAL_CONST_CORRECT_MARSHAL({
                     base_type::marshal( archive);
                  })
               };


            } // connect
         } // server



      } // message
   } //common
} // casual


#endif // SERVER_H_
