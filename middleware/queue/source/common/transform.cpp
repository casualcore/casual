//!
//! casual
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

                     result.process.pid = group.process.pid;
                     result.process.queue = group.process.queue;

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

            auto queue_type = []( common::message::queue::information::Queue::Type type )
                  {
                     switch( type)
                     {
                        case common::message::queue::information::Queue::Type::group_error_queue: return broker::admin::Queue::Type::group_error_queue;
                        case common::message::queue::information::Queue::Type::error_queue: return broker::admin::Queue::Type::error_queue;
                        default: return broker::admin::Queue::Type::queue;
                     }
                  };

            result.id = queue.id;
            result.name = queue.name;
            result.retries = queue.retries;
            result.type = queue_type( queue.type);
            result.error = queue.error;

            result.count = queue.count;
            result.size = queue.size;
            result.uncommitted = queue.uncommitted;
            result.timestamp = queue.timestamp;

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
            result.attributes.available = value.avalible;
            result.attributes.properties = value.properties;
            result.attributes.reply = value.reply;
            result.payload.type = value.type;
            std::swap( result.payload.data, value.payload);

            return result;
         }

         broker::admin::Message Message::operator () ( const common::message::queue::information::Message& message) const
         {
            broker::admin::Message result;

            result.id = message.id;
            result.queue = message.queue;
            result.origin = message.origin;
            result.reply = message.reply;
            result.trid = message.trid;
            result.type = message.type;

            result.state = message.state;
            result.redelivered = message.redelivered;

            result.avalible = message.avalible;
            result.timestamp = message.timestamp;

            result.size = message.size;


            return result;
         }

         std::vector< broker::admin::Message> messages( const common::message::queue::information::messages::Reply& reply)
         {
            return common::range::transform( reply.messages, Message{});
         }



      } // transform
   } // qeue


} // casual
