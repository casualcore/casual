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
                  if( ! entity.alias || entity.alias.value().empty())
                     entity.alias = configuration::alias::generate::placeholder();
               };
            } // alias



         } // <unnamed>
      } // local


      namespace transaction
      {
         Manager normalize( Manager manager)
         {
            if( ! manager.defaults)
               return manager;

            auto update_resource = [&defaults = manager.defaults.value().resource]( auto& resource)
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
                     group.limit.value().size = algorithm::coalesce( group.limit.value().size, limit.size);
                     group.limit.value().messages = algorithm::coalesce( group.limit.value().messages, limit.messages);
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

         Manager normalize( Manager manager)
         {
            if( manager.inbound)
               manager.inbound = normalize( std::move( manager.inbound.value()));

            if( manager.outbound)
               manager.outbound = normalize( std::move( manager.outbound.value()));

            if( manager.reverse)
            {
               if( manager.reverse.value().inbound)
                  manager.reverse.value().inbound = normalize( std::move( manager.reverse.value().inbound.value()));
               if( manager.reverse.value().outbound)
                  manager.reverse.value().outbound = normalize( std::move( manager.reverse.value().outbound.value()));
            }

            // the rest is deprecated...
            if( ! manager.defaults)
               return manager;

            // TODO deprecated remove on 2.0
            if( manager.defaults.value().connection && manager.connections)
            {
               log::line( log::category::warning, code::casual::invalid_configuration, " domain.gateway.default.connection is deprecated - there is no replacement");

               auto update_connection = [&defaults = manager.defaults.value().connection.value()]( auto& connection)
               {
                  connection.address = algorithm::coalesce( connection.address, defaults.address);
                  connection.restart = connection.restart.value_or( defaults.restart);
               };
               algorithm::for_each( manager.connections.value(), update_connection);
            }

            // TODO deprecated remove on 2.0
            if( manager.defaults.value().listener && manager.listeners)
            {
               log::line( log::category::warning, code::casual::invalid_configuration, " domain.gateway.default.listener is deprecated - use domain.gateway.inbound.default");

               auto update_listener = [&defaults = manager.defaults.value().listener.value()]( auto& listener)
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
                     if( service.reply && ! service.reply.value().delay && default_service.reply)
                        service.reply.value().delay = default_service.reply.value().delay;
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
                        queue.target.delay = default_queue.target.value().delay;
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
                     queue.retry.value().count = algorithm::coalesce( std::move( queue.retry.value().count), std::move( queue.retries));
                  else
                     queue.retry = Queue::Retry{ std::move( queue.retries), std::nullopt};
               }
            };

            // normalize defaults
            if( manager.defaults)
            {
               // queue
               if( manager.defaults.value().queue)
                  normalize_queue_retry( manager.defaults.value().queue.value());
            }
            

            // normalize groups
            if( manager.groups)
            {
               auto normalize = [&]( auto& group)
               {
                  if( group.name)
                     log::line( log::category::warning, "configuration - domain.queue.groups[].name is deprecated - use domain.queue.groups[].alias instead");

                  group.alias = algorithm::coalesce( std::move( group.alias), std::move( group.name));

                  algorithm::for_each( group.queues, [&]( auto& queue)
                  {
                     local::validate::not_empty( queue.name, "domain.queue.groups[].queues[].name");
                     
                     normalize_queue_retry( queue);

                     // should we complement retry with defaults?
                     if( manager.defaults && manager.defaults.value().queue)
                     {
                        if( queue.retry && manager.defaults.value().queue.value().retry)
                        {
                           queue.retry.value().count = algorithm::coalesce( std::move( queue.retry.value().count), manager.defaults.value().queue.value().retry.value().count);
                           queue.retry.value().delay = algorithm::coalesce( std::move( queue.retry.value().delay), manager.defaults.value().queue.value().retry.value().delay);
                        }
                        else
                           queue.retry = algorithm::coalesce( std::move( queue.retry), manager.defaults.value().queue.value().retry);
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

      Domain normalize( Domain domain)
      {
         Trace trace{ "configuration::user::Domain::normalize"};

         algorithm::for_each( domain.executables, local::alias::placeholder);
         algorithm::for_each( domain.servers, local::alias::placeholder);

         auto apply_normalize = []( auto& manager)
         {
            if( manager)
               manager = normalize( std::move( manager.value()));
         };

         apply_normalize( domain.transaction);
         apply_normalize( domain.gateway);
         apply_normalize( domain.queue);

         // validate? - no, we don't 'validate' user model

         if( ! domain.defaults)
            return domain; // nothing to normalize

         if( domain.defaults.value().environment)
         {
            log::line( log::category::warning, "configuration - domain.default.environment is deprecated - use domain.environment instead");
            domain.environment = algorithm::coalesce( std::move( domain.environment), std::move( domain.defaults.value().environment));
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
            algorithm::for_each( entities, normalize_entity);
         };

         if( domain.defaults.value().server)
            normalize_entities( domain.servers, domain.defaults.value().server.value());

         if( domain.defaults.value().executable)
            normalize_entities( domain.executables, domain.defaults.value().executable.value());

         if( domain.defaults.value().service)
         {
            auto normalize_service = [&defaults = domain.defaults.value().service.value()]( auto& service)
            {
               if( defaults.timeout)
                  service.timeout = service.timeout.value_or( defaults.timeout.value());

               if( defaults.execution)
               {
                  if( !service.execution) 
                     service.execution = service.execution.emplace();

                  if( defaults.execution.value().timeout)
                  {
                     if( !service.execution.value().timeout) 
                        service.execution.value().timeout = service.execution.value().timeout.emplace();

                     if( defaults.execution.value().timeout.value().duration)
                        service.execution.value().timeout.value().duration =  
                           service.execution.value().timeout.value().duration.value_or( defaults.execution.value().timeout.value().duration.value());

                     if( defaults.execution.value().timeout.value().contract)
                        service.execution.value().timeout.value().contract =  
                           service.execution.value().timeout.value().contract.value_or( defaults.execution.value().timeout.value().contract.value());

                  }
               }
               
            };
            algorithm::for_each( domain.services, normalize_service);
         }

         return domain;
      }

   } // configuration::user
} // casual