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
            common::server::Arguments services( manager::State& state);

         } // admin
      } // manager
   } // admin

} // casual

#endif // CASUAL_GATEWAY_MANAGER_ADMIN_SERVER_H_
