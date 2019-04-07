//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/message/pending.h"

namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace pending
         {

            bool Message::sent() const
            {
               return destinations.empty();
            }

            Message::operator bool () const
            {
               return sent();
            }

            void Message::remove( strong::ipc::id ipc)
            {
               algorithm::trim( destinations, algorithm::remove( destinations, ipc));
            }

            void Message::remove( strong::process::id pid)
            {
               algorithm::trim( destinations, algorithm::remove( destinations, pid));
            }

            std::ostream& operator << ( std::ostream& out, const Message& value)
            {
               return out << "{ destinations: " << value.destinations << ", complete: " << value.complete << "}";
            }

            namespace non
            {
               namespace blocking
               {
                  bool send( Message& message, const communication::error::type& handler)
                  {
                     auto send = [&]( const common::process::Handle& process)
                     {
                        try
                        {
                           communication::ipc::outbound::Device device{ process.ipc};
                           return static_cast< bool>( device.put( message.complete, communication::ipc::policy::non::Blocking{}, handler));
                        }
                        catch( const exception::system::communication::Unavailable&)
                        {
                           return true;
                        }
                     };

                     algorithm::trim( message.destinations, algorithm::remove_if( message.destinations, send));
                  
                     return message.sent();
                  }

               } // blocking
            } // non

         } // pending
      } // message
   } // common
} // casual
