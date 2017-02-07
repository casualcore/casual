//!
//! casual
//!

#ifndef QUEUE_BROKER_BROKER_H_
#define QUEUE_BROKER_BROKER_H_


#include "queue/broker/state.h"


#include "common/message/queue.h"
#include "common/transaction/id.h"



#include <vector>

namespace casual
{
   namespace queue
   {
      namespace broker
      {
         struct Settings
         {
            std::string group_executable;
         };


         namespace message
         {
            void pump( State& state);

         } // message

         std::vector< common::message::queue::information::queues::Reply> queues( State& state);

         common::message::queue::information::messages::Reply messages( State& state, const std::string& queue);

      } // broker

      struct Broker
      {
         Broker( broker::Settings settings);
         ~Broker();

         void start();

      private:

         broker::State m_state;

      };



   } // queue
} // casual

#endif // BROKER_H_
