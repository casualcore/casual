//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


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

         manager::admin::State::Remote remote( const manager::State& state);

         struct Message
         {
            queue::Message operator () ( common::message::queue::dequeue::Reply::Message& value) const;

            manager::admin::Message operator () ( const common::message::queue::information::Message& message) const;
         };

         std::vector< manager::admin::Message> messages( const common::message::queue::information::messages::Reply& reply);




      } // transform
   } // qeue


} // casual


