//!
//! casual
//!

#ifndef CASUAL_QUEUE_BROKER_ADMIN_SERVER_H_
#define CASUAL_QUEUE_BROKER_ADMIN_SERVER_H_


#include "queue/manager/admin/queuevo.h"

#include "common/server/argument.h"

namespace casual
{
   namespace queue
   {
      namespace manager
      {
         struct State;

         namespace admin
         {
            admin::State state( manager::State& state);

            std::vector< Message> list_messages( manager::State& state, const std::string& queue);

            std::vector< Affected> restore( manager::State& state, const std::string& queue);

            common::server::Arguments services( manager::State& state);

         } // manager
      } // manager
   } // queue


} // casual

#endif // SERVER_H_