//!
//! transform.h
//!
//! Created on: Oct 4, 2014
//!     Author: Lazan
//!

#ifndef CASUAL_QUEUE_BROKER_ADMIN_TRANSFORM_H_
#define CASUAL_QUEUE_BROKER_ADMIN_TRANSFORM_H_

#include "queue/broker/admin/queuevo.h"
#include "queue/api/message.h"

#include "common/message/queue.h"


namespace casual
{
   namespace queue
   {

      namespace broker
      {
         struct State;
         struct Queues;

      } // broker


      namespace transform
      {

         std::vector< broker::admin::Group> groups( const broker::State& state);


         struct Queue
         {
            broker::admin::Queue operator () ( const common::message::queue::information::Queue& queue) const;
         };

         std::vector< broker::admin::Queue> queues( const std::vector< common::message::queue::information::queues::Reply>& values);

         struct Message
         {
            queue::Message operator () ( common::message::queue::dequeue::Reply::Message& value) const;

            broker::admin::Message operator () ( const common::message::queue::information::Message& message) const;
         };

         std::vector< broker::admin::Message> messages( const common::message::queue::information::messages::Reply& reply);




      } // transform
   } // qeue


} // casual

#endif // TRANSFORM_H_
