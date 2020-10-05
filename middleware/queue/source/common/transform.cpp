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

         manager::admin::model::Forward forward( std::vector< common::message::queue::forward::state::Reply> values)
         {
            manager::admin::model::Forward result;

            auto add_reply = [&result]( auto& reply)
            {
               auto is_alias = []( auto& alias)
               {
                  return [&alias]( auto& value){ return value.alias == alias;};
               };

               auto assign_common = []( auto& source, auto& target)
               {
                  target.alias = std::move( source.alias);
                  target.source.name = std::move( source.source.name);
                  target.target.name = std::move( source.target.name);
                  target.instances.configured = source.instances.configured;
                  target.instances.running = source.instances.running;
                  target.metric.commit.count = source.metric.commit.count;
                  target.metric.commit.last = source.metric.commit.last;
                  target.metric.rollback.count = source.metric.rollback.count;
                  target.metric.rollback.last = source.metric.rollback.last;
                  target.note = std::move( source.note);
               };

               auto update_common = []( auto& source, auto& target)
               {
                  target.instances.configured += source.instances.configured;
                  target.instances.running += source.instances.running;

                  target.metric.commit.count += source.metric.commit.count;
                  target.metric.commit.last = std::max( target.metric.commit.last, source.metric.commit.last);
                  target.metric.rollback.count += source.metric.rollback.count;
                  target.metric.rollback.last = std::max( target.metric.rollback.last, source.metric.rollback.last);
               };

               auto add_service = [&]( auto& service)
               {
                  auto& services = result.services;

                  if( auto found = algorithm::find_if( services, is_alias( service.alias)))
                  {
                     update_common( service, *found);
                  }
                  else
                  {
                     manager::admin::model::Forward::Service model;
                     assign_common( service, model);

                     if( service.reply)
                     {
                        manager::admin::model::Forward::Service::Reply reply;
                        reply.name = service.reply.value().name;
                        reply.delay = service.reply.value().delay;
                        model.reply = std::move( reply);
                     }

                     services.push_back( std::move( model));
                  }
               };

               auto add_queue = [&]( auto& queue)
               {
                  auto& queues = result.queues;

                  if( auto found = algorithm::find_if( queues, is_alias( queue.alias)))
                  {
                     update_common( queue, *found);
                  }
                  else
                  {
                     manager::admin::model::Forward::Queue model;
                     assign_common( queue, model);
                     model.target.delay = queue.target.delay;

                     queues.push_back( std::move( model));
                  }
               };

               algorithm::for_each( reply.services, add_service);
               algorithm::for_each( reply.queues, add_queue);


            };

            algorithm::for_each( values, add_reply);

            return result;
         }

         common::message::queue::forward::configuration::Reply forward( manager::admin::model::Forward model)
         {
            common::message::queue::forward::configuration::Reply result;

            auto assign_common = []( auto& source, auto& target)
            {
               target.alias = std::move( source.alias);
               target.source.name = std::move( source.source.name);
               target.target.name = std::move( source.target.name);
               target.instances = source.instances.configured;
               target.note = std::move( source.note);
            };

            result.services = algorithm::transform( model.services, [&]( auto& forward)
            {
               common::message::queue::forward::configuration::Reply::Service result;
               assign_common( forward, result);
               if( forward.reply)
               {
                  common::message::queue::forward::configuration::Reply::Service::Reply reply;
                  reply.name = std::move( forward.reply.value().name);
                  reply.delay = forward.reply.value().delay;
                  result.reply = std::move( reply);
               }
               
               return result;
            });

            result.queues = algorithm::transform( model.queues, [&]( auto& forward)
            {
               common::message::queue::forward::configuration::Reply::Queue result;
               assign_common( forward, result);
               result.target.delay = forward.target.delay;
               return result;
            });


            return result;
         }
      } // transform
   } // queue
} // casual
