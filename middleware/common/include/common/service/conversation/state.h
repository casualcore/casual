//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "casual/platform.h"
#include "common/service/descriptor.h"

#include "common/message/conversation.h"

#include "common/serialize/macro.h"

namespace casual
{

   namespace common
   {
      namespace service
      {
         namespace conversation
         {
            namespace state
            {
               namespace descriptor
               {
                  struct Information
                  {
                     enum class Duplex : short
                     {
                        send,
                        receive,
                        terminated
                     };


                     message::conversation::Route route;

                     Duplex duplex = Duplex::receive;
                     bool initiator = false;

                     inline friend std::ostream& operator << ( std::ostream& out, Duplex value)
                     {
                        switch( value)
                        {
                           case Duplex::receive: return out << "receive";
                           case Duplex::send: return out << "send";
                           case Duplex::terminated: return out << "terminated";
                        }
                        return out << "unknown...";
                     }
                     
                     // for loging
                     CASUAL_CONST_CORRECT_SERIALIZE_WRITE(
                     {
                        CASUAL_SERIALIZE( route);
                        CASUAL_SERIALIZE( duplex);
                        CASUAL_SERIALIZE( initiator);
                     })
                  };

               } // descriptor

            } // state

            struct State
            {
               using holder_type =  service::descriptor::Holder< state::descriptor::Information>;
               using descriptor_type = typename holder_type::descriptor_type;

               holder_type descriptors;
            };

         } // conversation

      } // service
   } // common

} // casual


