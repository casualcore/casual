//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "queue/manager/transform.h"

namespace casual
{
   using namespace common;
   namespace queue::manager::transform
   {
      namespace model
      {
         admin::model::State state(
            const manager::State& state,
            std::vector< ipc::message::group::state::Reply> groups,
            std::vector< ipc::message::forward::group::state::Reply> forwards)
         {
            Trace trace{ "common::queue::manager::transform::model::state"};
            log::line( verbose::log, "groups: ", groups);
            log::line( verbose::log, "forwards: ", forwards);

            admin::model::State result;

            // groups
            for( auto& group : groups)
            {
               result.groups.push_back( admin::model::Group{ std::move( group.alias), group.process, std::move( group.queuebase), std::move( group.note)});

               auto transform_queue = [&group]( auto& queue)
               {
                  admin::model::Queue result;
                  result.group = group.process.pid;
                  result.name = queue.name;
                  result.id = queue.id;
                  result.retry.count = queue.retry.count;
                  result.retry.delay = queue.retry.delay;
                  result.error = queue.error;
                  result.count = queue.metric.count;
                  result.size = queue.metric.size;
                  result.uncommitted = queue.metric.uncommitted;
                  result.metric.dequeued = queue.metric.dequeued;
                  result.metric.enqueued = queue.metric.enqueued;
                  result.last = queue.metric.last;
                  result.created = queue.created;
                  return result;
               };

               algorithm::transform_if( group.queues, 
                  std::back_inserter( result.queues), 
                  transform_queue,
                  [&group]( auto& queue){ return !algorithm::find( group.zombies, queue.id);});
 
               algorithm::transform_if( group.queues, 
                  std::back_inserter( result.zombies), 
                  transform_queue,
                  [&group]( auto& queue){ return algorithm::find( group.zombies, queue.id);});
 
            }
            
            
            // forward
            for( auto& forward : forwards)
            {
               auto transform_metric = []( auto& entity)
               {
                  auto transform_count = []( auto& value)
                  {
                     return admin::model::forward::Metric::Count{ value.count, value.last};
                  };
                  admin::model::forward::Metric result;
                  result.commit = transform_count( entity.commit);
                  result.rollback = transform_count( entity.rollback);
                  return result;
               };

               result.forward.groups.push_back( admin::model::forward::Group{ std::move( forward.alias), forward.process, std::move( forward.note)});

               auto transform_service = [&forward, &transform_metric]( auto& service)
               {
                  auto transform_reply = []( auto& reply) -> std::optional< admin::model::forward::Service::Reply>
                  {
                     if( ! reply)
                        return {};
                     
                     return admin::model::forward::Service::Reply{
                        reply.value().queue, 
                        reply.value().delay
                     };
                  };

                  return admin::model::forward::Service{
                     forward.process.pid,
                     std::move( service.alias),
                     std::move( service.source),
                     admin::model::forward::Service::Target{ service.target.service},
                     admin::model::forward::Instances{ service.instances.configured, service.instances.running},
                     transform_reply( service.reply),
                     transform_metric( service.metric),
                     std::move(  service.note)
                  };
               };

               algorithm::transform( forward.services, std::back_inserter( result.forward.services), transform_service);

               auto transform_queue = [&forward, &transform_metric]( auto& queue)
               {
                  return admin::model::forward::Queue{
                     forward.process.pid,
                     std::move( queue.alias),
                     std::move( queue.source),
                     admin::model::forward::Queue::Target{ queue.target.queue, queue.target.delay},
                     admin::model::forward::Instances{ queue.instances.configured, queue.instances.running},
                     transform_metric( queue.metric),
                     std::move(  queue.note)
                  };
               };

               algorithm::transform( forward.queues, std::back_inserter( result.forward.queues), transform_queue);

            }

            // find remote queues and add to model
            for( auto [ queue_name, queue_instances] : state.queues)
            {
               auto remote_queues = algorithm::filter( queue_instances, []( const auto& instance) { return instance.remote();});

               algorithm::for_each( remote_queues, [&result, &queue_name]( const auto& remote_queue)
               {
                  result.remote.queues.push_back( admin::model::remote::Queue{ queue_name, remote_queue.process.pid});
               });
            }

            auto transform_remote_domains = []( const auto& remote)
            {
               return admin::model::remote::Domain{ remote.process, remote.order};
            };

            algorithm::transform( state.remotes, std::back_inserter( result.remote.domains), transform_remote_domains);
        
            return result;
         }

         namespace message
         {
            std::vector< admin::model::Message> meta( std::vector< ipc::message::group::message::meta::Reply> messages)
            {
               return algorithm::accumulate( messages, std::vector< admin::model::Message>{}, []( auto result, ipc::message::group::message::meta::Reply& message)
               {
                  algorithm::transform( message.messages, std::back_inserter( result), []( ipc::message::group::message::Meta& message)
                  {
                     auto transform_state = []( auto state)
                     {
                        return admin::model::Message::State( state);
                     };
                     admin::model::Message result;
                     result.id = message.id;
                     result.queue = message.queue;
                     result.origin = message.origin;
                     result.trid = message.trid;
                     result.state = transform_state( message.state);
                     result.reply = message.reply;
                     result.redelivered = message.redelivered;
                     result.type = message.type;
                     result.available = message.available;
                     result.timestamp = message.timestamp;
                     result.size = message.size;
                     return result;
                  });
                  return result;
               });
            }
            
         } // message
      } // model

      casual::configuration::model::queue::Model configuration( const State& state)
      {
         casual::configuration::model::queue::Model result;

         result.note = state.note;

         auto configuration = []( auto& group)
         {
            return group.configuration;
         };

         result.groups = algorithm::transform( state.groups, configuration);
         result.forward.groups = algorithm::transform( state.forward.groups, configuration);

         return result;
      }

   } // queue::manager::transform
} // casual