//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "configuration/user.h"
#include "configuration/common.h"

#include "common/algorithm.h"
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

         } // <unnamed>
      } // local


      namespace transaction
      {
         void Manager::normalize()
         {
            if( ! defaults)
               return;

            auto update_resource = [&defaults = defaults.value().resource]( auto& resource)
            {
               resource.key = coalesce( resource.key, defaults.key);
               resource.instances = coalesce( resource.instances, defaults.instances);
            };

            algorithm::for_each( resources, update_resource);
         }
      } // transaction

      namespace gateway
      {

         void Inbound::normalize()
         {
            Trace trace{ "configuration::user::gateway::Inbound::normalize"};

            auto local_default = defaults.value_or( inbound::Default{});

            // connections
            {
               auto default_connection = local_default.connection.value_or( inbound::Default::Connection{});
               for( auto& group : groups)
               {
                  algorithm::for_each( group.connections, [&default_connection]( auto& connection)
                  {
                     connection.discovery = coalesce( connection.discovery, default_connection.discovery);
                     local::validate::not_empty( connection.address, "inbound.groups[].address");
                  });
               }
            }

            if( local_default.limit)
            {
               auto limit = local_default.limit.value();

               for( auto& group : groups)
               {
                  if( group.limit)
                  {
                     group.limit.value().size = coalesce( group.limit.value().size, limit.size);
                     group.limit.value().messages = coalesce( group.limit.value().messages, limit.messages);
                  }
               }
            }
         }

         void Outbound::normalize()
         {
            Trace trace{ "configuration::user::gateway::Outbound::normalize"};

            for( auto& group : groups)
            {
               algorithm::for_each( group.connections, []( auto& connection)
               {
                  local::validate::not_empty( connection.address, "outbound.groups[].address");
               });
            }
         }

         void Manager::normalize()
         {
            if( inbound)
               inbound.value().normalize();

            if( outbound)
               outbound.value().normalize();

            if( reverse)
            {
               if( reverse.value().inbound)
                  reverse.value().inbound.value().normalize();
               if( reverse.value().outbound)
                  reverse.value().outbound.value().normalize();
            }

            // the rest is deprecated...
            if( ! defaults)
               return;

            // TODO deprecated remove on 2.0
            if( defaults.value().connection && connections)
            {
               log::line( log::category::warning, code::casual::invalid_configuration, " domain.gateway.default.connection is deprecated - there is no replacement");

               auto update_connection = [&defaults = defaults.value().connection.value()]( auto& connection)
               {
                  connection.address = coalesce( connection.address, defaults.address);
                  connection.restart = connection.restart.value_or( defaults.restart);
               };
               algorithm::for_each( connections.value(), update_connection);
            }

            // TODO deprecated remove on 2.0
            if( defaults.value().listener && listeners)
            {
               log::line( log::category::warning, code::casual::invalid_configuration, " domain.gateway.default.listener is deprecated - use domain.gateway.inbound.default");

               auto update_listener = [&defaults = defaults.value().listener.value()]( auto& listener)
               {
                  if( ! listener.limit)
                  {
                     listener.limit = defaults.limit;
                     return;
                  }
                  auto& limit = listener.limit.value();
                  limit.size = coalesce( limit.size, defaults.limit.size);
                  limit.messages = coalesce( limit.messages, defaults.limit.messages);
               };
               algorithm::for_each( listeners.value(), update_listener);
            }

         }

      } // gateway

      namespace queue
      {
         void Forward::normalize()
         {
            if( ! defaults || ! groups)
               return;

            auto& forward_default = defaults.value();

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

            algorithm::for_each( groups.value(), normalize);
         }

         void Manager::normalize()
         {
            auto normalize_queue_retry = []( auto& queue)
            {
               if( queue.retries)
               {
                  log::line( log::category::warning, "configuration - queue.retries is deprecated - use queue.retry.count instead");
                  
                  if( queue.retry)
                     queue.retry.value().count = coalesce( std::move( queue.retry.value().count), std::move( queue.retries));
                  else
                     queue.retry = Queue::Retry{ std::move( queue.retries), std::nullopt};
               }
            };

            // normalize defaults
            if( defaults)
            {
               // queue
               if( defaults.value().queue)
                  normalize_queue_retry( defaults.value().queue.value());
            }
            

            // normalize groups
            if( groups)
            {
               auto normalize = [&]( auto& group)
               {
                  if( group.name)
                     log::line( log::category::warning, "configuration - domain.queue.groups[].name is deprecated - use domain.queue.groups[].alias instead");

                  group.alias = coalesce( std::move( group.alias), std::move( group.name));

                  algorithm::for_each( group.queues, [&]( auto& queue)
                  {
                     local::validate::not_empty( queue.name, "domain.queue.groups[].queues[].name");
                     
                     normalize_queue_retry( queue);

                     // should we complement retry with defaults?
                     if( defaults && defaults.value().queue)
                     {
                        if( queue.retry && defaults.value().queue.value().retry)
                        {
                           queue.retry.value().count = coalesce( std::move( queue.retry.value().count), defaults.value().queue.value().retry.value().count);
                           queue.retry.value().delay = coalesce( std::move( queue.retry.value().delay), defaults.value().queue.value().retry.value().delay);
                        }
                        else
                           queue.retry = coalesce( std::move( queue.retry), defaults.value().queue.value().retry);
                     }
                  });
                  
               };
               
               algorithm::for_each( groups.value(), normalize);
            }

            // normalize service forward
            if( forward)
               forward.value().normalize();

         }
      } // queue

      void Domain::normalize()
      {
         Trace trace{ "configuration::user::Domain::normalize"};


         auto alias_placeholder = []( auto& entity)
         {
            if( ! entity.alias || entity.alias.value().empty())
               entity.alias = alias::generate::placeholder();
         };

         algorithm::for_each( executables, alias_placeholder);
         algorithm::for_each( servers, alias_placeholder);

         auto normalize = []( auto& manager)
         {
            if( manager)
               manager.value().normalize();
         };

         normalize( transaction);
         normalize( gateway);
         normalize( queue);

         // validate? - no, we don't 'validate' user model

         if( ! defaults)
            return; // nothing to normalize

         if( defaults.value().environment)
         {
            log::line( log::category::warning, "configuration - domain.default.environment is deprecated - use domain.environment instead");
            environment = coalesce( std::move( environment), std::move( defaults.value().environment));
         }

         
         auto normalize_entities = []( auto& entities, auto& defaults)
         {
            auto normalize_entity = [&defaults]( auto& entity)
            {
               entity.instances = entity.instances.value_or( defaults.instances);
               entity.memberships = coalesce( entity.memberships, defaults.memberships);
               entity.environment = coalesce( entity.environment, defaults.environment);
               entity.restart = entity.restart.value_or( defaults.restart);
            };
            algorithm::for_each( entities, normalize_entity);
         };

         if( defaults.value().server)
            normalize_entities( servers, defaults.value().server.value());

         if( defaults.value().executable)
            normalize_entities( executables, defaults.value().executable.value());

         if( defaults.value().service)
         {
            auto normalize_service = [&defaults = defaults.value().service.value()]( auto& service)
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
            algorithm::for_each( services, normalize_service);
         }

      }

   } // configuration::user
} // casual