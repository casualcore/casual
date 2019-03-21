//! 
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "common/message/type.h"
#include "common/communication/message.h"
#include "common/marshal/complete.h"

namespace casual
{
   namespace retry
   {
      namespace send
      {
         namespace message
         {
            struct Request : common::message::basic_message< common::message::Type::retry_send_message>
            {
               common::process::Handle destination;
               common::communication::message::Complete message;

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  archive & destination;
                  archive & message;
               })
               friend std::ostream& operator << ( std::ostream& out, const Request& rhs);
            };

            void send( const Request& request);


            template< typename M>
            void send( M&& message, const common::process::Handle& destination)
            {
               Request request;
               request.destination = destination;
               request.message = common::marshal::complete( std::forward< M>( message));
               send( request);
            }

         } // message         

      } // send
      
   } // retry
} // casual