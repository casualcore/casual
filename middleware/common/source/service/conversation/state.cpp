//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/service/conversation/state.h"


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

                  std::ostream& operator << ( std::ostream& out, const Information::Duplex& value)
                  {
                     switch( value)
                     {
                        case Information::Duplex::receive: return out << "receive";
                        case Information::Duplex::send: return out << "send";
                        case Information::Duplex::terminated: return out << "terminated";
                        default: return out << "unknown...";
                     }
                  }

                  std::ostream& operator << ( std::ostream& out, const Information& value)
                  {
                     return out << "{ route: " << value.route
                        << ", initiator: " << value.initiator
                        << ", duplex: " << value.duplex
                        << '}';
                           
                  }

               } // descriptor

            } // state

            State::State() = default;


         } // conversation

      } // service
   } // common

} // casual
