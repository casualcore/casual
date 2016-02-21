//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_DELAY_MESSAGE_H_
#define CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_DELAY_MESSAGE_H_


#include "common/message/type.h"

#include "common/communication/message.h"
#include "common/marshal/complete.h"

#include "common/uuid.h"
#include "common/process.h"

#include <chrono>

namespace casual
{
   namespace domain
   {
      namespace delay
      {
         const common::Uuid& identification();

         namespace message
         {
            struct Request : common::message::basic_message< common::message::Type::delay_message>
            {
               common::platform::queue_id_type destination;
               std::chrono::microseconds delay;
               common::communication::message::Complete message;

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  archive & destination;
                  archive & delay;
                  archive & message;
               })
            };

            void send( const Request& request);


            template< typename M, typename R, typename D>
            void send( M&& message, common::platform::queue_id_type destination, std::chrono::duration< R, D> delay)
            {
               Request request;
               request.destination = destination;
               request.delay = std::chrono::duration_cast< std::chrono::microseconds>( delay);
               request.message = common::marshal::complete( std::move( message));

               send( request);

            }

         } // message

      } // delay

   } // domain


} // casual

#endif // CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_DELAY_MESSAGE_H_
