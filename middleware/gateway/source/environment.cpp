//!
//! casual 
//!

#include "gateway/environment.h"
#include "common/environment.h"


namespace casual
{
   namespace gateway
   {
      namespace environment
      {
         const common::Uuid& identification()
         {
            static const common::Uuid id{ "b9624e2f85404480913b06e8d503fce5"};
            return id;
         }

         namespace variable
         {
            namespace name
            {
               namespace manager
               {

                  const std::string& queue()
                  {
                     static std::string singleton{ "CASUAL_GATEWAY_MANAGER_QUEUE"};
                     return singleton;
                  }
               } // manager


            } // name

         } // variable


         namespace manager
         {


            common::communication::ipc::outbound::Device& device()
            {
               static common::communication::ipc::outbound::Device singelton{
                  common::environment::variable::get< common::platform::ipc::id::type>( variable::name::manager::queue())};
               return singelton;
            }

            void set( common::platform::ipc::id::type queue)
            {
               common::environment::variable::set( variable::name::manager::queue(), queue);
            }


         } // manager

      } // environment
   } // gateway


} // casual
