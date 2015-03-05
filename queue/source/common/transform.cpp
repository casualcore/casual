//!
//! transform.cpp
//!
//! Created on: Oct 4, 2014
//!     Author: Lazan
//!

#include "queue/common/transform.h"
#include "queue/broker/broker.h"

#include "common/algorithm.h"


namespace casual
{
   namespace queue
   {

      namespace transform
      {

         namespace local
         {
            namespace
            {

               struct Group
               {
                  broker::admin::Group operator () ( const broker::State::Group& group) const
                  {
                     broker::admin::Group result;

                     result.id.pid = group.process.pid;
                     result.id.queue = group.process.queue;

                     result.name = group.name;
                     result.queuebase = group.queuebase;

                     return result;
                  }
               };

            } // <unnamed>
         } // local




         std::vector< broker::admin::Group> groups( const broker::State& state)
         {
            std::vector< broker::admin::Group> result;

            common::range::transform( state.groups, result, local::Group{});

            return result;
         }

         broker::admin::Queue Queue::operator () ( const common::message::queue::information::Queue& queue) const
         {
            broker::admin::Queue result;

            result.id = queue.id;
            result.name = queue.name;
            result.retries = queue.retries;
            result.type = queue.type;
            result.error = queue.error;

            result.message.counts = queue.message.counts;
            result.message.timestamp = queue.message.timestamp;
            result.message.size.min = queue.message.size.min;
            result.message.size.max = queue.message.size.max;
            result.message.size.average = queue.message.size.average;
            result.message.size.total = queue.message.size.total;


            return result;
         }

         std::vector< broker::admin::Queue> queues( const std::vector< common::message::queue::information::queues::Reply>& values)
         {
            std::vector< broker::admin::Queue> result;

            for( auto& value : values)
            {
               auto range = common::range::transform( value.queues, result, transform::Queue{});

               common::range::for_each( range, [&]( broker::admin::Queue& q){
                  q.group = value.process.pid;
               });
            }

            return result;
         }




         queue::Message Message::operator () ( common::message::queue::dequeue::Reply::Message& value) const
         {
            queue::Message result;

            result.id = value.id;
            result.attribues.available = value.avalible;
            result.attribues.properties = value.correlation;
            result.attribues.reply = value.reply;
            result.payload.type.type = value.type.type;
            result.payload.type.subtype = value.type.subtype;
            std::swap( result.payload.data, value.payload);

            return result;
         }




      } // transform
   } // qeue


} // casual
