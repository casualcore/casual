//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "queue/manager/admin/model.h"
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

         std::vector< manager::admin::model::Group> groups( const manager::State& state);


         struct Queue
         {
            manager::admin::model::Queue operator () ( const common::message::queue::information::Queue& queue) const;
         };

         std::vector< manager::admin::model::Queue> queues( const std::vector< common::message::queue::information::queues::Reply>& values);

         manager::admin::model::State::Remote remote( const manager::State& state);

         struct Message
         {
            queue::Message operator () ( common::message::queue::dequeue::Reply::Message& value) const;

            manager::admin::model::Message operator () ( const common::message::queue::information::Message& message) const;
         };

         std::vector< manager::admin::model::Message> messages( const common::message::queue::information::messages::Reply& reply);

         manager::admin::model::Forward forward( std::vector< common::message::queue::forward::state::Reply> values);

         //! We know we just have one instance of the forward for now...
         common::message::queue::forward::configuration::Reply forward( manager::admin::model::Forward model);




      } // transform
   } // qeue
} // casual


