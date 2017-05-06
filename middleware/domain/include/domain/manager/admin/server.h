//!
//! casual
//!

#ifndef CASUAL_GATEWAY_MANAGER_ADMIN_SERVER_H_
#define CASUAL_GATEWAY_MANAGER_ADMIN_SERVER_H_



#include "common/server/argument.h"

namespace casual
{
   namespace domain
   {

      namespace manager
      {
         struct State;

         namespace admin
         {
            namespace service
            {
               namespace name
               {
                  constexpr auto state() { return ".casual.domain.state";}
                  namespace scale
                  {
                     constexpr auto instances() { return ".casual.domain.scale.instances";}
                  } // scale

                  constexpr auto shutdown() { return ".casual.domain.shutdown";}

                  namespace configuration
                  {
                     constexpr auto persist() { return ".casual/domain/configuration/persist";}

                  } // configuration

               } // name
            } // service

            common::server::Arguments services( manager::State& state);


         } // admin
      } // manager
   } // admin

} // casual

#endif // CASUAL_GATEWAY_MANAGER_ADMIN_SERVER_H_
