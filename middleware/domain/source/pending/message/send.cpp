//! 
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "domain/pending/message/send.h"
#include "domain/pending/message/environment.h"
#include "domain/common.h"

#include "common/communication/instance.h"

namespace casual
{
   using namespace common;

   namespace domain
   {
      namespace pending
      {
         namespace message
         {
            
            namespace local
            {
               namespace
               {
                  auto& device() 
                  {
                     static communication::instance::outbound::detail::Device device{ environment::identification, environment::variable}; 
                     return device;  
                  }

                  struct Request : common::message::basic_message< common::message::Type::domain_pending_send_request>
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

                  void send( const Request& request, const common::communication::error::type& handler)
                  {
                     communication::ipc::blocking::send( local::device(), request, handler);
                  }
                  
               } // <unnamed>
            } // local

            void send( const common::message::pending::Message& message, const common::communication::error::type& handler)
            {
               Trace trace{ "domain::pending::send"};

               log::line( verbose::log, "message: ", message);

               local::send( local::Request{ message}, handler);
            }   
         } // message
      } // pending
   } // domain
} // casual