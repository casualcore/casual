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
            std::vector< ipc::message::forward::group::state::Reply> forwards,
            std::vector< ipc::message::fanout::group::state::Reply> fanouts)
         {
            Trace trace{ "common::queue::manager::transform::model::state"};
            log::debug( "groups: ", groups);
            log::debug( "forwards: ", forwards);
            log::debug( "fanouts: ", fanouts);

            admin::model::State result;

            auto alias_less_than = []( const auto& lhs, const auto& rhs){ return lhs.alias < rhs.alias;};

            // groups
            for( auto& group : groups)
            {
               result.groups.push_back(
                  admin::model::Group{ std::move( group.alias), group.process, std::move( group.queuebase), std::move( group.note), { group.size.current, group.size.capacity}}
               );

               auto transform_queue = [ &state, &group]( auto& queue)
               {
                  auto find_lookup = [ &]( auto& queue) -> const manager::state::Queue*
                  {
                     if( auto pair = algorithm::find( state.queues, queue.name))
                        if( auto lookup = algorithm::find( pair->second, group.process.ipc))
                           return pair->second.data();

                     return nullptr;
                  };

                  admin::model::Queue result;
                  result.group = group.process.pid;
                  result.name = queue.name;
                  result.id = queue.id;

                  if( auto lookup = find_lookup( queue))
                     result.enable = admin::model::Queue::Enable{ .enqueue = lookup->enable.enqueue, .dequeue = lookup->enable.dequeue};

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
                  [ &group]( auto& queue){ return ! algorithm::find( group.zombies, queue.id);});
 
               algorithm::transform_if( group.queues, 
                  std::back_inserter( result.zombies), 
                  transform_queue,
                  [ &group]( auto& queue){ return algorithm::find( group.zombies, queue.id);});
            }

            std::ranges::sort( result.groups, alias_less_than);
            
            
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

               result.forward.groups.push_back(
                  admin::model::forward::Group{ std::move( forward.alias), forward.process, std::move( forward.note)}
               );

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
                     std::move( service.note),
                     service.enabled
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
                     std::move(  queue.note),
                     queue.enabled
                  };
               };

               algorithm::transform( forward.queues, std::back_inserter( result.forward.queues), transform_queue);
            }

            // fanout
            for( auto& fanout : fanouts)
            {
               result.fanout.groups.push_back(
                  admin::model::fanout::Group{ 
                     .alias = std::move( fanout.alias), 
                     .process = fanout.process, 
                     .note = std::move( fanout.note)
                  });

               auto transform_queue = [ &fanout]( auto& queue)
               {
                  auto transform_target = []( auto& target)
                  {
                     return admin::model::fanout::Queue::Target{ 
                        .queue = std::move( target.queue), 
                        .delay = target.delay
                     };
                  };

                  return admin::model::fanout::Queue{
                     .group = fanout.process.pid,
                     .alias = std::move( queue.alias),
                     .source = std::move( queue.source),
                     .targets = algorithm::transform( queue.targets, transform_target),
                     .instances = admin::model::fanout::Instances{ 
                        .configured = queue.instances.configured, 
                        .running = queue.instances.running, 
                        .stopped = queue.instances.stopped
                     },
                     .metric = admin::model::fanout::Metric{ 
                        .commit = { 
                           .count = queue.metric.commit.count, 
                           .last = queue.metric.commit.last},
                        .rollback = { 
                           .count = queue.metric.rollback.count, 
                           .last = queue.metric.rollback.last}
                     },
                     .note = std::move(  queue.note),
                     .enabled = queue.enabled
                  };
               };

               algorithm::transform( fanout.queues, std::back_inserter( result.fanout.queues), transform_queue);
            }

            std::ranges::sort( result.forward.groups, alias_less_than);

            // find remote queues and add to model
            for( auto& queue : state.queues)
            {
               algorithm::transform_if( queue.second, std::back_inserter( result.remote.queues), [ &queue]( auto& instance)
               {
                  return admin::model::remote::Queue{ queue.first, instance.process};
               },
               []( auto& instance)
               {
                  return instance.remote();
               });
            }

            // remotes

            auto transform_remote_domains = []( const auto& remote)
            {
               return admin::model::remote::Domain{ 
                  .alias = remote.alias, 
                  .process = remote.process, 
                  .order = remote.order, 
                  .description = remote.description,
                  .reservations = remote.reservations};
            };

            algorithm::transform( state.remotes, std::back_inserter( result.remote.domains), transform_remote_domains);

            // sort for deterministic output
            {
               std::ranges::sort( result.groups, {}, &admin::model::Group::alias);
               std::ranges::sort( result.queues, {}, &admin::model::Queue::name);
               std::ranges::sort( result.zombies, {}, &admin::model::Queue::name);
               std::ranges::sort( result.forward.groups, {}, &admin::model::forward::Group::alias);
               std::ranges::sort( result.forward.services, {}, &admin::model::forward::Service::alias);
               std::ranges::sort( result.forward.queues, {}, &admin::model::forward::Queue::alias);
               std::ranges::sort( result.fanout.groups, {}, &admin::model::fanout::Group::alias);
               std::ranges::sort( result.fanout.queues, {}, &admin::model::fanout::Queue::alias);
               std::ranges::sort( result.remote.domains, {}, &admin::model::remote::Domain::alias);
               std::ranges::sort( result.remote.queues, {}, &admin::model::remote::Queue::name);
            }
        
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
         result.fanout.groups = algorithm::transform( state.fanout.groups, configuration);

         return result;
      }

   } // queue::manager::transform
} // casual
