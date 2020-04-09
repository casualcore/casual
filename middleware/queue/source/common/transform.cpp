//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "queue/common/transform.h"
#include "queue/manager/manager.h"

#include "common/algorithm.h"


namespace casual
{
   using namespace common;
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
                  manager::admin::model::Group operator () ( const manager::State::Group& group) const
                  {
                     manager::admin::model::Group result;

                     result.process = group.process;

                     result.name = group.name;
                     result.queuebase = group.queuebase;

                     return result;
                  }
               };

            } // <unnamed>
         } // local


         std::vector< manager::admin::model::Group> groups( const manager::State& state)
         {
            std::vector< manager::admin::model::Group> result;

            common::algorithm::transform( state.groups, result, local::Group{});

            return result;
         }

         manager::admin::model::Queue Queue::operator () ( const common::message::queue::information::Queue& queue) const
         {
            manager::admin::model::Queue result;

            result.id = queue.id;
            result.name = queue.name;
            result.retry.count = queue.retry.count;
            result.retry.delay = queue.retry.delay;
            result.error = queue.error;

            result.count = queue.count;
            result.size = queue.size;
            result.uncommitted = queue.uncommitted;
            result.metric.enqueued = queue.metric.enqueued;
            result.metric.dequeued = queue.metric.dequeued;
            result.last = queue.last;
            result.created = queue.created;

            return result;
         }

         std::vector< manager::admin::model::Queue> queues( const std::vector< common::message::queue::information::queues::Reply>& values)
         {
            std::vector< manager::admin::model::Queue> result;

            for( auto& value : values)
            {
               auto range = common::algorithm::transform( value.queues, result, transform::Queue{});

               common::algorithm::for_each( range, [&]( manager::admin::model::Queue& q){
                  q.group = value.process.pid;
               });
            }

            return result;
         }

         manager::admin::model::State::Remote remote( const manager::State& state)
         {
            manager::admin::model::State::Remote result;

            common::algorithm::transform( state.remotes, result.domains, []( auto& r){
               manager::admin::model::remote::Domain domain;
               
               domain.process = r.process;
               domain.order = r.order;

               return domain;
            });

            for( auto& queue : state.queues)
            {
               auto found = common::algorithm::find_if( queue.second, []( auto& i){
                  return i.order > 0;
               });

               common::algorithm::transform( found, result.queues, [&queue]( auto& i){
                  manager::admin::model::remote::Queue result;
                  result.name = queue.first;
                  result.pid = i.process.pid;

                  return result;
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

         manager::admin::model::Message Message::operator () ( const common::message::queue::information::Message& message) const
         {
            manager::admin::model::Message result;

            auto transform_state = []( auto state)
            {
               using Enum = decltype( state);
               switch( state)
               {
                  case Enum::enqueued: return manager::admin::model::Message::State::enqueued;
                  case Enum::committed: return manager::admin::model::Message::State::committed;
                  case Enum::dequeued: return manager::admin::model::Message::State::dequeued;
               }
               // will not happend - the compiler will give error if we not handle all enums...
               return manager::admin::model::Message::State::dequeued;
            };

            result.id = message.id;
            result.queue = message.queue;
            result.origin = message.origin;
            result.reply = message.reply;
            result.trid = message.trid;
            result.type = message.type;

            result.state = transform_state( message.state);
            result.redelivered = message.redelivered;

            result.available = message.available;
            result.timestamp = message.timestamp;

            result.size = message.size;


            return result;
         }

         std::vector< manager::admin::model::Message> messages( const common::message::queue::information::messages::Reply& reply)
         {
            return common::algorithm::transform( reply.messages, Message{});
         }

         namespace forward
         {
            configuration::model::queue::Forward configuration( std::vector< ipc::message::forward::state::Reply> values)
            {
               configuration::model::queue::Forward result;

               auto add_reply = [&result]( auto& reply)
               {
                  algorithm::transform( reply.services, std::back_inserter( result.services), []( auto& service)
                  {
                     configuration::model::queue::forward::Service result;
                     result.alias = std::move( service.alias);
                     result.instances = service.instances.configured;
                     result.note = std::move( service.note);
                     if( service.reply)
                     {
                        configuration::model::queue::forward::Service::Reply reply;
                        reply.queue = service.reply.value().queue;
                        reply.delay = service.reply.value().delay;
                        result.reply = std::move( reply);
                     }
                     
                     return result;
                  });
               };

               algorithm::for_each( values, add_reply);

               return result;
            }

         
            manager::admin::model::Forward state( std::vector< ipc::message::forward::state::Reply> values)
            {
               manager::admin::model::Forward result;

               auto add_reply = [&result]( auto& reply)
               {
                  auto assign_common = []( auto& source, auto& target)
                  {
                     target.alias = std::move( source.alias);
                     target.source = std::move( source.source);
                     target.instances.configured = source.instances.configured;
                     target.instances.running = source.instances.running;
                     target.metric.commit.count = source.metric.commit.count;
                     target.metric.commit.last = source.metric.commit.last;
                     target.metric.rollback.count = source.metric.rollback.count;
                     target.metric.rollback.last = source.metric.rollback.last;
                     target.note = std::move( source.note);
                  };

                  algorithm::transform( reply.services, std::back_inserter( result.services), [&assign_common]( auto& service)
                  {
                     manager::admin::model::Forward::Service result;
                     assign_common( service, result);
                     result.target.service = service.target.service;

                     if( service.reply)
                     {
                        manager::admin::model::Forward::Service::Reply reply;
                        reply.queue = service.reply.value().queue;
                        reply.delay = service.reply.value().delay;
                        result.reply = std::move( reply);
                     }
                     
                     return result;
                  });

                  algorithm::transform( reply.queues, std::back_inserter( result.queues), [&assign_common]( auto& queue)
                  {
                     manager::admin::model::Forward::Queue result;
                     assign_common( queue, result);
                     result.target.queue = queue.target.queue;
                     result.target.delay = queue.target.delay;
                     return result;
                  });
               };

               algorithm::for_each( values, add_reply);

               return result;
            }
         } // forward

      } // transform
   } // queue
} // casual
