//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_DELAY_DELAY_H_
#define CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_DELAY_DELAY_H_


#include "domain/delay/message.h"

#include "common/platform.h"
#include "common/communication/message.h"

namespace casual
{
   namespace domain
   {
      namespace delay
      {
         struct Settings
         {

         };



         struct State
         {
            State( Settings settings) {}

            struct Message
            {
               common::platform::ipc::id::type destination;
               common::communication::message::Complete message;
               common::platform::time_point deadline;

               friend bool operator < ( const Message& lhs, const Message& rhs);
            };

            void add( message::Request&& message);
            std::vector< Message> passed( common::platform::time_point time);
            std::vector< Message> passed() { return passed( common::platform::clock_type::now());}

            std::chrono::microseconds timeout() const;


         private:
            std::vector< Message> m_messages;


         };



         void start( State state);

         namespace message
         {
            void pump( State& state);
         } // message

         int main( int argc, char **argv);

      } // delay
   } // domain
} // casual

#endif // CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_DELAY_DELAY_H_
