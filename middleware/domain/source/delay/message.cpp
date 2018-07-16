//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "domain/delay/message.h"

#include "common/communication/ipc.h"
#include "common/communication/instance.h"
#include "common/chronology.h"

namespace casual
{
   using namespace common;

   namespace domain
   {
      namespace delay
      {

         const common::Uuid& identification()
         {
            static common::Uuid id{ "c3209cf4627447d6a5e385a0ef08e4e7"};
            return id;
         }

         namespace local
         {
            namespace
            {
               namespace process
               {
                  common::process::Handle fetch()
                  {
                     return common::communication::instance::fetch::handle(
                           identification(), common::communication::instance::fetch::Directive::wait);
                  }

                  common::process::Handle& get()
                  {
                     static auto handle = fetch();
                     return handle;
                  }

                  common::process::Handle& reset()
                  {
                     get() = fetch();
                     return get();
                  }

               } // process


               void send( const message::Request& request, const communication::ipc::outbound::Device::error_type error_handler = nullptr)
               {
                  while( true)
                  {
                     try
                     {
                        communication::ipc::blocking::send( local::process::get().ipc, request, error_handler);
                        return;
                     }
                     catch( const exception::system::communication::Unavailable&)
                     {
                        local::process::reset();
                     }
                  }
               }


            } // <unnamed>
         } // local


         namespace message
         {
            void send( const Request& request)
            {
               local::send( request);
            }

            std::ostream& operator << ( std::ostream& out, const Request& rhs)
            {
               out << "{ destination: " << rhs.destination
                  << ", delay: "; common::chronology::format{}( out, rhs.delay);
               out << ", message: " << rhs.message
                  << '}';
               return out;
            }

         } // message

      } // delay
   } // domain
} // casual

