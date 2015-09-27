//!
//! server.h
//!
//! Created on: Sep 30, 2014
//!     Author: Lazan
//!

#ifndef CASUAL_QUEUE_BROKER_ADMIN_SERVER_H_
#define CASUAL_QUEUE_BROKER_ADMIN_SERVER_H_


#include "queue/broker/admin/queuevo.h"

#include "common/server/argument.h"

namespace casual
{
   namespace queue
   {
      struct Broker;


      namespace broker
      {
         struct State;

         namespace admin
         {
            admin::State list_queues( broker::State& state);

            std::vector< Message> list_messages( broker::State& state, const std::string& queue);

            common::server::Arguments services( broker::State& state);

         } // admin
      } // broker
   } // queue


} // casual

#endif // SERVER_H_
