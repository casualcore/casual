//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "domain/delay/message.h"

#include "casual/platform.h"
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
               common::strong::ipc::id destination;
               common::communication::message::Complete message;
               platform::time::point::type deadline;

               friend bool operator < ( const Message& lhs, const Message& rhs);
            };

            void add( message::Request&& message);
            std::vector< Message> passed( platform::time::point::type time);
            std::vector< Message> passed() { return passed( platform::time::clock::type::now());}

            platform::time::unit timeout() const;


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


