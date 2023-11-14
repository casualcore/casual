//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "configuration/user.h"
#include "configuration/common.h"

#include "common/algorithm.h"
#include "common/algorithm/coalesce.h"
#include "common/string.h"
#include "common/code/raise.h"
#include "common/code/casual.h"
#include "common/log/line.h"



namespace casual
{
   using namespace common;
   namespace configuration::user
   {
      namespace local
      {
         namespace
         {

            namespace validate
            {
               template< typename T>
               auto not_empty( T&& value, std::string_view role)
               {
                  if( value.empty())
                     code::raise::error( code::casual::invalid_configuration, role, " has to have a value");
               }

            } // validate

            namespace alias
            {
               auto placeholder = []( auto& entity)
               {
                  if( ! entity.alias || entity.alias->empty())
                     entity.alias = configuration::alias::generate::placeholder();
               };
            } // alias

            //! If `optional` not contains a value, a default constructed value
            //! is _inserted_ .
            //! @returns `*optional`
            template< typename T>
            auto value_or_emplace( std::optional< T>& optional) -> decltype( optional.emplace())
            {
               if( optional)
                  return *optional;

               return optional.emplace();
            }

         } // <unnamed>
      } // local

      namespace system
      {
         Model normalize( Model model)
         {
            return model;
         }
      } // system

      namespace domain
      {

         namespace transaction
         {
            Manager normalize( Manager manager)
            {
               if( ! manager.defaults)
                  return manager;

               auto update_resource = [ &defaults = manager.defaults->resource]( auto& resource)
               {
                  resource.key = algorithm::coalesce( resource.key, defaults.key);
                  resource.instances = algorithm::coalesce( resource.instances, defaults.instances);
               };

               algorithm::for_each( manager.resources, update_resource);

               return manager;
            }
         } // transaction

         namespace gateway
         {

            Inbound normalize( Inbound inbound)
            {
               Trace trace{ "configuration::user::gateway::Inbound::normalize"};

               auto local_default = inbound.defaults.value_or( inbound::Default{});

               // connections
               {
                  auto default_connection = local_default.connection.value_or( inbound::Default::Connection{});
                  for( auto& group : inbound.groups)
                     for( auto& connection : group.connections)
                     {
                        connection.discovery = algorithm::coalesce( connection.discovery, default_connection.discovery);
                        local::validate::not_empty( connection.address, "inbound.groups[].address");
                     };
                  
               }

               if( local_default.limit)
               {
                  auto limit = local_default.limit.value();

                  for( auto& group : inbound.groups)
                     if( group.limit)
                     {
                        group.limit->size = algorithm::coalesce( group.limit->size, limit.size);
                        group.limit->messages = algorithm::coalesce( group.limit->messages, limit.messages);
                     }
               }
               return inbound;
            }

            Outbound normalize( Outbound outbound)
            {
               Trace trace{ "configuration::user::gateway::Outbound::normalize"};

               for( auto& group : outbound.groups)
                  for( auto& connection : group.connections)
                     local::validate::not_empty( connection.address, "outbound.groups[].address");

               return outbound;
            }

            namespace deprecated
            {
               Manager normalize( Manager manager)
               {
                  // deprecated listeners
                  if( manager.listeners && ! manager.listeners->empty())
                  {
                     log::warning( code::casual::invalid_configuration, "domain.gateway.listeners[] is deprecated - use domain.gateway.inbound.groups[]");

                     gateway::inbound::Group group;
                     group.alias = "inbound";
                     group.note = "transformed from DEPRECATED domain.gateway.listeners[]";

                     group.connections = common::algorithm::transform( *manager.listeners, []( auto& listener)
                     {
                        gateway::inbound::Connection result;
                        if( listener.note)
                           result.note = std::move( *listener.note);
                        result.address = std::move( listener.address);
                        return result;
                     });

                     auto accumulate_limit = []( auto current, auto& listener)
                     {
                        if( ! listener.limit)
                           return current;

                        if( listener.limit->size)
                           local::value_or_emplace( current).size = 
                              std::max( listener.limit->size.value_or( 0), local::value_or_emplace( current).size.value_or( 0));

                        if( listener.limit->messages)
                           local::value_or_emplace( current).messages = 
                              std::max( listener.limit->messages.value_or( 0), local::value_or_emplace( current).messages.value_or( 0));

                        return current;
                     };

                     group.limit = algorithm::accumulate( *manager.listeners, std::optional< gateway::inbound::Limit>{}, accumulate_limit);

                     local::value_or_emplace( manager.inbound).groups.push_back( std::move( group)); 

                     // remove it...
                     manager.listeners = {};
                  }

                  // deprecated connections
                  if( manager.connections && ! manager.connections->empty())
                  {
                     log::warning( code::casual::invalid_configuration, "domain.gateway.connections[ is deprecated - use domain.gateway.outbound.groups[]");

                     common::algorithm::transform( *manager.connections, local::value_or_emplace( manager.outbound).groups, []( gateway::Connection& value)
                     {
                        gateway::outbound::Group result;
                        result.note = "transformed from DEPRECATED domain.gateway.connections[]";
                        
                        gateway::outbound::Connection connection;
                        
                        connection.note = std::move( value.note);
                        connection.address = std::move( value.address);
                     
                        if( value.services)
                           connection.services = *value.services;
                        if( value.queues)
                           connection.queues = *value.queues;

                        result.connections.push_back( std::move( connection));

                        return result;
                     });

                     // remove it...
                     manager.connections = {};
                  }

                  if( ! manager.defaults)
                     return manager;

                  // TODO deprecated remove on 2.0
                  if( manager.defaults->connection && manager.connections)
                  {
                     log::error( code::casual::invalid_configuration, "domain.gateway.default.connection is deprecated/removed - there is no replacement");

                     auto update_connection = [ &defaults = *manager.defaults->connection]( auto& connection)
                     {
                        connection.address = algorithm::coalesce( connection.address, defaults.address);
                        connection.restart = connection.restart.value_or( defaults.restart);
                     };
                     algorithm::for_each( manager.connections.value(), update_connection);
                  }

                  // either way, remove it
                  manager.defaults->connection = {};

                  // TODO deprecated remove on 2.0
                  if( manager.defaults->listener && manager.listeners)
                  {
                     log::warning( code::casual::invalid_configuration, "domain.gateway.default.listener is deprecated - use domain.gateway.inbound.default");

                     auto update_listener = [&defaults = manager.defaults->listener.value()]( auto& listener)
                     {
                        if( ! listener.limit)
                        {
                           listener.limit = defaults.limit;
                           return;
                        }
                        auto& limit = listener.limit.value();
                        limit.size = algorithm::coalesce( limit.size, defaults.limit.size);
                        limit.messages = algorithm::coalesce( limit.messages, defaults.limit.messages);
                     };
                     algorithm::for_each( manager.listeners.value(), update_listener);
                  }

                  // either way, remove it
                  manager.defaults->listener = {};

                  

                  return manager;
               }
            } // deprecated

            Manager normalize( Manager manager)
            {
               // take care of deprecated stuff...
               manager = deprecated::normalize( std::move( manager));

               if( manager.inbound)
                  manager.inbound = normalize( std::move( *manager.inbound));

               if( manager.outbound)
                  manager.outbound = normalize( std::move( *manager.outbound));

               if( manager.reverse)
               {
                  if( manager.reverse->inbound)
                     manager.reverse->inbound = normalize( std::move( *manager.reverse->inbound));
                  if( manager.reverse->outbound)
                     manager.reverse->outbound = normalize( std::move( *manager.reverse->outbound));
               }

               return manager;
            }

         } // gateway

         namespace queue
         {
            Forward normalize( Forward forward)
            {
               if( ! forward.defaults || ! forward.groups)
                  return forward;

               auto& forward_default = forward.defaults.value();

               auto normalize = [&]( auto& group)
               {
                  if( group.services && forward_default.service)
                  { 
                     auto& default_service = forward_default.service.value();
                     algorithm::for_each( group.services.value(), [&]( auto& service)
                     {
                        if( ! service.instances)
                           service.instances = default_service.instances;
                        if( service.reply && ! service.reply->delay && default_service.reply)
                           service.reply->delay = default_service.reply->delay;
                     });
                  }

                  if( group.queues && forward_default.queue)
                  {
                     auto& default_queue = forward_default.queue.value();
                     algorithm::for_each( group.queues.value(), [&]( auto& queue)
                     {
                        if( ! queue.instances)
                           queue.instances = default_queue.instances;
                        if( ! queue.target.delay && default_queue.target)
                           queue.target.delay = default_queue.target->delay;
                     });
                  }
               };

               algorithm::for_each( forward.groups.value(), normalize);

               return forward;
            }

            Manager normalize( Manager manager)
            {
               auto normalize_queue_retry = []( auto& queue)
               {
                  if( queue.retries)
                  {
                     log::line( log::category::warning, "configuration - queue.retries is deprecated - use queue.retry.count instead");
                     
                     if( queue.retry)
                        queue.retry->count = algorithm::coalesce( std::move( queue.retry->count), std::move( queue.retries));
                     else
                        queue.retry = Queue::Retry{ std::move( queue.retries), std::nullopt};
                  }
               };

               // normalize defaults
               if( manager.defaults)
               {
                  // queue
                  if( manager.defaults->queue)
                     normalize_queue_retry( manager.defaults->queue.value());
               }
               

               // normalize groups
               if( manager.groups)
               {
                  auto normalize = [&]( auto& group)
                  {
                     if( group.name)
                        log::warning( code::casual::invalid_configuration, "configuration - domain.queue.groups[].name is deprecated - use domain.queue.groups[].alias instead");

                     group.alias = algorithm::coalesce( std::move( group.alias), std::move( group.name));

                     algorithm::for_each( group.queues, [&]( auto& queue)
                     {
                        local::validate::not_empty( queue.name, "domain.queue.groups[].queues[].name");
                        
                        normalize_queue_retry( queue);

                        // should we complement retry with defaults?
                        if( manager.defaults && manager.defaults->queue)
                        {
                           if( queue.retry && manager.defaults->queue->retry)
                           {
                              queue.retry->count = algorithm::coalesce( std::move( queue.retry->count), manager.defaults->queue->retry->count);
                              queue.retry->delay = algorithm::coalesce( std::move( queue.retry->delay), manager.defaults->queue->retry->delay);
                           }
                           else
                              queue.retry = algorithm::coalesce( std::move( queue.retry), manager.defaults->queue->retry);
                        }
                     });
                     
                  };
                  
                  algorithm::for_each( manager.groups.value(), normalize);
               }

               // normalize service forward
               if( manager.forward)
                  manager.forward = normalize( std::move( manager.forward.value()));

               return manager;

            }
         } // queue

         Model normalize( Model model)
         {
            Trace trace{ "configuration::user::domain::Model::normalize"};

            if( model.executables)
               algorithm::for_each( *model.executables, local::alias::placeholder);
            if( model.servers)
               algorithm::for_each( *model.servers, local::alias::placeholder);

            auto apply_normalize = []( auto& manager)
            {
               if( manager)
                  manager = normalize( std::move( *manager));
            };

            apply_normalize( model.transaction);
            apply_normalize( model.gateway);
            apply_normalize( model.queue);


            // global
            {
               if( model.service)
               {
                  auto get_global_service = []( Model& model) -> decltype( auto)
                  {
                     return local::value_or_emplace( local::value_or_emplace( model.global).service);
                  };

                  // use of deprecated config
                  if( model.service->execution && model.service->execution->timeout 
                     && ( model.service->execution->timeout->duration || model.service->execution->timeout->contract))
                  {
                     log::warning( code::casual::invalid_configuration, "domain.service.timeout.(duration|contract) is deprecated - use domain.global.service.timeout.(duration|contract) instead");
                     local::value_or_emplace( get_global_service( model).execution).timeout = std::move( model.service->execution->timeout);
                  }

                  // Note: domain.service.discoverable is a new _artifact_ of reusing user model types, and we don't
                  // need to be backward compatible with it, we just ignore it. 
               }
            }
 
            if( model.services)
            {
               auto normalize_service = []( user::domain::Service& service)
               {
                  if( service.timeout)
                  {
                     log::warning( code::casual::invalid_configuration, "domain.services[].timeout is deprecated - use domain.services[].execution.duration instead");

                     if( service.execution && service.execution->timeout && service.execution->timeout->duration)
                     {
                        log::error( code::casual::invalid_configuration, 
                           "ambiguity - domain.services[].timeout is used the same time as domain.services[].execution.duration - remove domain.services[].timeout");
                     }
                     else
                     {
                        local::value_or_emplace( local::value_or_emplace( service.execution).timeout).duration = std::move( service.timeout);
                     }
                  }
               };

               algorithm::for_each( *model.services, normalize_service);
            }


            // defaults
            {
               if( model.defaults && model.defaults->service && model.defaults->service->timeout)
               {
                  log::warning( code::casual::invalid_configuration, 
                     "domain.default.service.timeout is deprecated - use domain.default.service.execution.duration instead");
                  local::value_or_emplace( local::value_or_emplace( model.defaults->service->execution).timeout).duration 
                     = std::move( model.defaults->service->timeout);
               }
            }

            if( ! model.defaults)
               return model; // nothing to normalize

            if( model.defaults->environment)
            {
               log::warning( code::casual::invalid_configuration, "domain.default.environment is deprecated - use domain.environment instead");
               model.environment = algorithm::coalesce( std::move( model.environment), std::move( model.defaults->environment));
            }

            
            auto normalize_entities = []( auto& entities, auto& defaults)
            {
               auto normalize_entity = [&defaults]( auto& entity)
               {
                  entity.instances = entity.instances.value_or( defaults.instances);
                  entity.memberships = algorithm::coalesce( entity.memberships, defaults.memberships);
                  entity.environment = algorithm::coalesce( entity.environment, defaults.environment);
                  entity.restart = entity.restart.value_or( defaults.restart);
               };

               if( entities)
                  algorithm::for_each( *entities, normalize_entity);
            };

            if( model.defaults->server)
               normalize_entities( model.servers, *model.defaults->server);

            if( model.defaults->executable)
               normalize_entities( model.executables, *model.defaults->executable);

            if( model.defaults->service)
            {
               auto normalize_service = [ &defaults = *model.defaults->service]( auto& service)
               {
                  if( defaults.timeout)
                     service.timeout = service.timeout.value_or( *defaults.timeout);

                  if( defaults.execution)
                  {
                     if( ! service.execution) 
                        service.execution = service.execution.emplace();

                     if( defaults.execution->timeout)
                     {
                        if( ! service.execution->timeout) 
                           service.execution->timeout = service.execution->timeout.emplace();

                        if( defaults.execution->timeout->duration)
                           service.execution->timeout->duration =  
                              service.execution->timeout->duration.value_or( *defaults.execution->timeout->duration);

                        if( defaults.execution->timeout->contract)
                           service.execution->timeout->contract =  
                              service.execution->timeout->contract.value_or( *defaults.execution->timeout->contract);

                     }
                  }

                  if( defaults.visibility && ! service.visibility)
                     service.visibility = defaults.visibility;
               };

               if( model.services)
                  algorithm::for_each( *model.services, normalize_service);
            }

            return model;
         }

      } // domain




      Model normalize( Model model)
      {
         Trace trace{ "configuration::user::Model::normalize"};

         if( model.system)
            model.system = normalize( std::move( *model.system));

         if( model.domain)
            model.domain = normalize( std::move( *model.domain));

         return model;
      }

   } // configuration::user
} // casual
