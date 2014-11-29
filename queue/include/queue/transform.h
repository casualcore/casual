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

         std::vector< broker::admin::GroupVO> groups( const broker::State& state);


         struct Queue
         {
            broker::admin::QueueVO operator () ( const common::message::queue::Queue& queue) const;
         };

         struct Group
         {
            broker::admin::verbose::GroupVO operator () ( const broker::Queues& queues) const;
         };



         struct Message
         {
            queue::Message operator () ( common::message::queue::dequeue::Reply::Message& value) const;
         };


      } // transform
   } // qeue


} // casual

#endif // TRANSFORM_H_
