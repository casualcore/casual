//!
//! casual
//!

#include "queue/common/transform.h"
#include "queue/manager/manager.h"

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
                  manager::admin::Group operator () ( const manager::State::Group& group) const
                  {
                     manager::admin::Group result;

                     result.process.pid = group.process.pid;
                     result.process.queue = group.process.queue.native();

                     result.name = group.name;
                     result.queuebase = group.queuebase;

                     return result;
                  }
               };

            } // <unnamed>
         } // local


         std::vector< manager::admin::Group> groups( const manager::State& state)
         {
            std::vector< manager::admin::Group> result;

            common::range::transform( state.groups, result, local::Group{});

            return result;
         }

         manager::admin::Queue Queue::operator () ( const common::message::queue::information::Queue& queue) const
         {
            manager::admin::Queue result;

            auto queue_type = []( common::message::queue::information::Queue::Type type )
                  {
                     switch( type)
                     {
                        case common::message::queue::information::Queue::Type::group_error_queue: return manager::admin::Queue::Type::group_error_queue;
                        case common::message::queue::information::Queue::Type::error_queue: return manager::admin::Queue::Type::error_queue;
                        default: return manager::admin::Queue::Type::queue;
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

         std::vector< manager::admin::Queue> queues( const std::vector< common::message::queue::information::queues::Reply>& values)
         {
            std::vector< manager::admin::Queue> result;

            for( auto& value : values)
            {
               auto range = common::range::transform( value.queues, result, transform::Queue{});

               common::range::for_each( range, [&]( manager::admin::Queue& q){
                  q.group = value.process.pid;
               });
            }

            return result;
         }




         queue::Message Message::operator () ( common::message::queue::dequeue::Reply::Message& value) const
         {
            queue::Message result;

            result.id = value.id;
            result.attributes.available = value.available;
            result.attributes.properties = value.properties;
            result.attributes.reply = value.reply;
            result.payload.type = value.type;
            std::swap( result.payload.data, value.payload);

            return result;
         }

         manager::admin::Message Message::operator () ( const common::message::queue::information::Message& message) const
         {
            manager::admin::Message result;

            result.id = message.id;
            result.queue = message.queue;
            result.origin = message.origin;
            result.reply = message.reply;
            result.trid = message.trid;
            result.type = message.type;

            result.state = message.state;
            result.redelivered = message.redelivered;

            result.available = message.available;
            result.timestamp = message.timestamp;

            result.size = message.size;


            return result;
         }

         std::vector< manager::admin::Message> messages( const common::message::queue::information::messages::Reply& reply)
         {
            return common::range::transform( reply.messages, Message{});
         }



      } // transform
   } // qeue


} // casual
