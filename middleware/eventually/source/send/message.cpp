//! 
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "eventually/send/message.h"
#include "eventually/send/environment.h"
#include "eventually/common.h"

#include "common/communication/instance.h"

namespace casual
{
   using namespace common;
   namespace eventually
   {
      namespace send
      {
         namespace local
         {
            namespace
            {
               auto& device() 
               {
                  static communication::instance::outbound::detail::Device device{ send::identification, send::environment}; 
                  return device;  
               }

               namespace optional
               {
                  auto& device() 
                  {
                     static communication::instance::outbound::detail::optional::Device device{ send::identification, send::environment}; 
                     return device;  
                  }
               } // optional


               struct Request : common::message::basic_message< common::message::Type::eventually_send_message>
               {
                  Request( const common::message::pending::Message& message)
                     : message( message) {}

                  const common::message::pending::Message& message;

                  template< typename A>
                  void marshal( A& archive) const
                  {
                     archive & message;
                  }
               };

               void send( const Request& request)
               {
                  communication::ipc::blocking::send( local::device(), request);
               }
               
            } // <unnamed>
         } // local

         namespace detail
         {
            std::ostream& operator << ( std::ostream& out, const Request& rhs)
            {
               return out << "{ message: " << rhs.message
                  << '}';
            }

            void message( const common::message::pending::Message& message)
            {
               communication::ipc::blocking::send( local::optional::device(), local::Request{ message});
            }
         } // detail 

         void message( const common::message::pending::Message& message)
         {
            Trace trace{ "eventually::send::message"};

            log::line( verbose::log, "message: ", message);

            local::send( local::Request{ message});
         }   

      } // send
      
   } // eventually
} // casual