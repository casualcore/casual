//!
//! broker.h
//!
//! Created on: Jun 20, 2014
//!     Author: Lazan
//!

#ifndef QUEUE_BROKER_BROKER_H_
#define QUEUE_BROKER_BROKER_H_


#include "common/message/queue.h"

#include "config/queue.h"

#include <string>
#include <unordered_map>

namespace casual
{
   namespace queue
   {
      namespace broker
      {
         struct Settings
         {
            std::string configuration;
         };

         struct State
         {
            struct Server
            {
               common::message::server::Id id;

            };

            std::vector< Server> servers;

            std::unordered_map< std::string, common::message::queue::lookup::Reply> queues;
         };
      } // broker

      struct Broker
      {
         Broker( broker::Settings settings);

         void start();

      private:

         broker::State m_state;

      };

   } // queue
} // casual

#endif // BROKER_H_
