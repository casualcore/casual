//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "configuration/user.h"
#include "configuration/common.h"

#include "common/algorithm.h"
#include "common/file.h"
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
            auto optional_empty = []( auto& value)
            {
               return ! value || value.value().empty();
            };
            namespace equal
            {
               auto name()
               {
                  return []( auto& l, auto& r){ return l.name == r.name;};
               }

               //! @note if both aliases ar _empty_ they ar not equal
               auto alias()
               {
                  return []( auto& l, auto& r)
                  { 
                     if( optional_empty( l.alias) && optional_empty( r.alias))
                        return false;
                        
                     return l.alias == r.alias;
                  };
               }

               auto address()
               {
                  return []( auto& l, auto& r){ return l.address == r.address;};
               }

               auto environment()
               {
                  return []( auto& l, auto& r)
                  { 
                     return l.key == r.key;
                  };
               }
            } // equal

            namespace validate
            {
               template< typename T>
               auto not_empty( T&& value, std::string_view role)
               {
                  if( value.empty())
                     code::raise::error( code::casual::invalid_configuration, role, " has to have a value");
               }

            } // validate   

            template< typename S, typename T, typename P>
            auto add_or_replace( S&& source, T& target, P predicate)
            {
               // remove all occurencies that is found in source, if any
               auto [ replaced, keep] = algorithm::intersection( target, source, predicate);
               
               if( replaced)
                  log::line( verbose::log, "replaced: ", replaced);

               algorithm::trim( target, keep);
               // ... and add all from source
               algorithm::move( std::forward< S>( source), target);
            };

            template< typename S, typename T, typename P>
            auto add_or_accumulate( S&& source, T& target, P predicate)
            {
               algorithm::for_each( source, [&target, predicate]( auto&& value)
               {
                  auto equal = [&value, predicate]( auto& other)
                  {
                     return predicate( value, other);
                  };

                  if( auto found = algorithm::find_if( target, equal))
                     (*found) += std::move( value);
                  else
                     target.push_back( std::move( value));
               });
               
            };

            template< typename S, typename T, typename P>
            auto optional_add( S&& source, T& target, P&& predicate)
            {
               if( target && source)
                  predicate( std::move( source.value()), target.value());
               else if( source)
                  target = coalesce( std::move( target), std::move( source));
            }

            template< typename S, typename T>
            auto optional_add( S&& source, T& target)
            {
               optional_add( std::forward< S>( source), target, []( auto&& source, auto& target)
               {  
                  target += std::move( source);
               });
            };

         } // <unnamed>
      } // local

      Environment& Environment::operator += ( Environment rhs)
      {
         local::optional_add( std::move( rhs.files), files, []( auto source, auto& target)
         { 
            local::add_or_replace( std::move( source), target, std::equal_to<>{});
         });
         local::optional_add( std::move( rhs.variables), variables, []( auto source, auto& target)
         { 
            local::add_or_replace( std::move( source), target, local::equal::environment());
         });
         return *this;
      }

      Service& Service::operator += ( Service rhs)
      {
         execution = coalesce( std::move( rhs.execution), std::move( execution));
         return *this;
      }

      namespace transaction
      {
         Manager& Manager::operator += ( Manager rhs)
         {
            log = coalesce( std::move( rhs.log), std::move( log));
            local::add_or_replace( std::move( rhs.resources), resources, local::equal::name());

            return *this;
         }

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

         namespace inbound
         {
            Connection& Connection::operator += ( Connection rhs)
            {
               note = coalesce( std::move( rhs.note), std::move( note));
               discovery = coalesce( std::move( rhs.discovery ), std::move( discovery));
               return *this;
            }

            Group& Group::operator += ( Group rhs)
            {
               local::add_or_replace( std::move( rhs.connections), connections, local::equal::address());
               return *this;
            }
            
         } // inbound

         namespace outbound
         {
            Connection& Connection::operator += ( Connection rhs)
            {
               auto merge = []( auto&& source, auto& target)
               {
                  algorithm::append_unique( source, target);
               };

               local::optional_add( std::move( rhs.services), services, merge);
               local::optional_add( std::move( rhs.queues), queues, merge);

               note = coalesce( std::move( rhs.note), std::move( note));
               return *this;
            }

            Group& Group::operator += ( Group rhs)
            {
               local::add_or_accumulate( std::move( rhs.connections), connections, local::equal::address());
               note = coalesce( std::move( rhs.note), std::move( note));
               return *this;
            }
         } // outbound

         Inbound& Inbound::operator += ( Inbound rhs)
         {
            local::add_or_accumulate( std::move( rhs.groups), groups, local::equal::alias());
            return *this;
         }

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

         Outbound& Outbound::operator += ( Outbound rhs)
         {
            local::add_or_accumulate( std::move( rhs.groups), groups, local::equal::alias());
            return *this;
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

         Reverse& Reverse::operator += ( Reverse rhs)
         {
            local::optional_add( std::move( rhs.inbound), inbound);
            local::optional_add( std::move( rhs.outbound), outbound);
            
            return *this;
         }

         Manager& Manager::operator += ( Manager rhs)
         {
            local::optional_add( std::move( rhs.listeners), listeners, []( auto source, auto& target)
            {
               local::add_or_replace( std::move( source), target, local::equal::address());
            });
            local::optional_add( std::move( rhs.connections), connections, []( auto source, auto& target)
            {
               local::add_or_replace( std::move( source), target, local::equal::address());
            });

            local::optional_add( std::move( rhs.inbound), inbound);
            local::optional_add( std::move( rhs.outbound), outbound);

            local::optional_add( std::move( rhs.reverse), reverse);

            return *this;
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
         namespace forward
         {
            Group& Group::operator += ( Group rhs)
            {
               assert( alias == rhs.alias);

               auto add_or_replace = []( auto&& source, auto& target)
               {
                  local::add_or_replace( std::move( source), target, local::equal::alias());
               };

               local::optional_add( std::move( rhs.services), services, add_or_replace);
               local::optional_add( std::move( rhs.queues), queues, add_or_replace);
               
               note = coalesce( std::move( rhs.note), std::move( note));

               return *this;
            }

         } // forward
         Forward& Forward::operator += ( Forward rhs)
         {
            auto add_or_accumulate = []( auto&& source, auto& target)
            {
               local::add_or_accumulate( std::move( source), target, local::equal::alias());
            };
            
            local::optional_add( std::move( rhs.groups), groups, add_or_accumulate);

            return *this;
         }

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

         Manager& Manager::operator += ( Manager rhs)
         {
            auto add_or_replace = []( auto&& source, auto& target)
            {
               local::add_or_replace( std::move( source), target, local::equal::alias());
            };

            local::optional_add( std::move( rhs.groups), groups, add_or_replace);
            local::optional_add( std::move( rhs.forward), forward);

            note = coalesce( std::move( rhs.note), std::move( note));

            return *this;
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

      namespace domain
      {
         Default& Default::operator += ( Default rhs)
         {
            service = coalesce( std::move( rhs.service), std::move( service));
            return *this;
         }
      } // domain

      Domain& Domain::operator += ( Domain rhs)
      {
         name = coalesce( std::move( rhs.name), std::move( name));
         note = coalesce( std::move( rhs.note), std::move( note));
         
         // we don't aggregate defaults.
         // local::optional_add( std::move( rhs.defaults), defaults);
         
         local::optional_add( std::move( rhs.environment), environment);

         local::optional_add( std::move( rhs.service), service);

         local::add_or_replace( std::move( rhs.groups), groups, local::equal::name());
         local::add_or_replace( std::move( rhs.servers), servers, local::equal::alias());
         local::add_or_replace( std::move( rhs.executables), executables, local::equal::alias());
         local::add_or_replace( std::move( rhs.services), services, local::equal::name());

         local::optional_add( rhs.transaction, transaction);
         local::optional_add( rhs.queue, queue);
         local::optional_add( rhs.gateway, gateway);
         
         return *this;
      }

      Domain operator + ( Domain lhs, Domain rhs)
      {
         lhs += std::move( rhs);
         return lhs;
      }

      void Domain::normalize()
      {
         Trace trace{ "configuration::user::Domain::normalize"};

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