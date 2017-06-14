//!
//! casual
//!

#ifndef CASUAL_QUEUE_BROKER_ADMIN_TRANSFORM_H_
#define CASUAL_QUEUE_BROKER_ADMIN_TRANSFORM_H_

#include "queue/manager/admin/queuevo.h"
#include "queue/api/message.h"

#include "common/message/queue.h"


namespace casual
{
   namespace queue
   {

      namespace manager
      {
         struct State;
         struct Queues;

      } // broker


      namespace transform
      {

         std::vector< manager::admin::Group> groups( const manager::State& state);


         struct Queue
         {
            manager::admin::Queue operator () ( const common::message::queue::information::Queue& queue) const;
         };

         std::vector< manager::admin::Queue> queues( const std::vector< common::message::queue::information::queues::Reply>& values);

         struct Message
         {
            queue::Message operator () ( common::message::queue::dequeue::Reply::Message& value) const;

            manager::admin::Message operator () ( const common::message::queue::information::Message& message) const;
         };

         std::vector< manager::admin::Message> messages( const common::message::queue::information::messages::Reply& reply);




      } // transform
   } // qeue


} // casual

#endif // TRANSFORM_H_
