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
   namespace configuration
   {
      namespace user
      {
         namespace local
         {
            namespace
            {
               namespace validate
               {
                  auto not_empty = []( auto& value, auto&& description)
                  {
                     if( value.empty())
                        code::raise::error( code::casual::invalid_configuration, description);
                  };

                  auto unique = []( auto& values, auto predicate, auto description)
                  {
                     auto range = range::make( values);

                     while( range.size() > 1)
                     {
                        auto is_same = [&current = range::front( range),predicate]( auto& value)
                        {
                           return predicate( current, value);
                        };
                        if( algorithm::find_if( range + 1, is_same))
                           code::raise::error( code::casual::invalid_configuration, description);
                        
                        ++range;
                     }
                  };
               } // validate
 

               auto optional_empty = []( auto& value)
               {
                  return ! value.has_value() || value.value().empty();
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

                  auto environment()
                  {
                     return []( auto& l, auto& r)
                     { 
                        return l.key == r.key;
                     };
                  }
               } // equal

               template< typename S, typename T, typename P>
               auto add_or_replace( S&& source, T& target, P predicate)
               {
                  // remove all occurencies that is found in source, if any
                  auto split = algorithm::intersection( target, source, predicate);
                  
                  if( std::get< 0>( split))
                     log::line( verbose::log, "replaced: ", std::get< 0>( split));

                  algorithm::trim( target, std::get< 1>( split));
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
                     target = std::move( source);
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
            local::add_or_replace( std::move( rhs.files), files, std::equal_to<>{});
            local::add_or_replace( std::move( rhs.variables), variables, local::equal::environment());
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
            Reverse& Reverse::operator += ( Reverse rhs)
            {
               auto add_or_replace = []( auto&& source, auto& target)
               {
                  local::add_or_replace( std::move( source), target, local::equal::alias());
               };

               local::optional_add( std::move( rhs.inbounds), inbounds, add_or_replace);
               local::optional_add( std::move( rhs.outbounds), outbounds, add_or_replace);
               
               return *this;
            }

            Manager& Manager::operator += ( Manager rhs)
            {
               auto equal_address = []( auto& l, auto& r){ return l.address == r.address;};

               local::add_or_replace( std::move( rhs.listeners), listeners, equal_address);
               local::add_or_replace( std::move( rhs.connections), connections, equal_address);

               auto append_replace_alias = []( auto&& source, auto& target)
               {
                  algorithm::append_replace( source, target, local::equal::alias());
               };

               local::optional_add( std::move( rhs.inbounds), inbounds, append_replace_alias);
               local::optional_add( std::move( rhs.outbounds), outbounds, append_replace_alias);


               local::optional_add( std::move( rhs.reverse), reverse);



               return *this;
            }  

            void Manager::normalize()
            {
               if( ! defaults)
                  return;

               // TODO deprecated remove on 2.0
               if( defaults.value().connection)
               {
                  log::line( log::category::warning, code::casual::invalid_configuration, " domain.gateway.default.connection is deprecated - there is no replacement");

                  auto update_connection = [&defaults = defaults.value().connection.value()]( auto& connection)
                  {
                     connection.address = coalesce( connection.address, defaults.address);
                     connection.restart = connection.restart.value_or( defaults.restart);
                  };
                  algorithm::for_each( connections, update_connection);
               }

               if( defaults.value().listener)
               {
                  log::line( log::category::warning, code::casual::invalid_configuration, " domain.gateway.default.listener is deprecated - use domain.gateway.default.inbound");

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
                  algorithm::for_each( listeners, update_listener);
               }

            }

         } // gateway

         namespace queue
         {
            Manager& Manager::operator += ( Manager rhs)
            {
               local::add_or_replace( std::move( rhs.groups), groups, local::equal::name());

               note = coalesce( std::move( rhs.note), std::move( note));

               return *this;
            }

            void Manager::normalize()
            {
               // validate
               {
                  auto group = []( auto& group)
                  {
                     local::validate::not_empty( group.name, "queue group has to have a name");

                     auto queue = []( auto& queue)
                     {
                        local::validate::not_empty( queue.name, "queue has to have a name");

                         if( queue.retries)
                           log::line( log::category::error, "configuration - queue.retries is deprecated - use queue.retry.count instead");
                     };

                     algorithm::for_each( group.queues, queue);
                  };
                  algorithm::for_each( groups, group);
               }

               if( defaults)
               {
                  auto group = [&defaults = defaults.value()]( auto& group)
                  {
                     if( local::optional_empty( group.queuebase))
                     {
                        if( defaults.directory.empty())
                           group.queuebase = string::compose( group.name, ".qb");
                        else 
                           group.queuebase = string::compose( defaults.directory, '/', group.name, ".qb");
                     }

                     if( ! defaults.queue.retry)
                        return;
                        
                     auto queue = [&retry = defaults.queue.retry.value()]( auto& queue)
                     {
                        if( ! queue.retry)
                        {
                           queue.retry = retry;
                           return;
                        }
                        
                        queue.retry.value().count = coalesce( queue.retry.value().count, retry.count);
                        queue.retry.value().delay = coalesce( queue.retry.value().delay, retry.delay);
                     };

                     algorithm::for_each( group.queues, queue);
                  };

                  algorithm::for_each( groups, group);
               }

               // Check unique groups
               {
                  struct Group
                  {
                     std::string name;
                     std::string queuebase;
                  };

                  // we just copy the stuff we want to comapre to not affect the original 'order' (might not matter)
                  auto groups = algorithm::transform( Manager::groups, []( auto& group)
                  {
                     Group result;
                     result.name = group.name;
                     result.queuebase = group.queuebase.value();
                     return result;
                  });
                  
                  auto name_order = []( auto& l, auto& r){ return l.name < r.name; };

                  if( common::algorithm::adjacent_find( common::algorithm::sort( groups, name_order), local::equal::name()))
                     code::raise::error( code::casual::invalid_configuration, "queue groups has to have unique names within a configuration file");

                  auto order_qb = []( auto& l, auto& r){ return l.queuebase < r.queuebase;};
                  auto equal_gb = []( auto& l, auto& r){ return l.queuebase == r.queuebase;};

                  // remove in-memory queues when we validate uniqueness
                  auto persitent = algorithm::filter( groups, []( auto& group){ return group.queuebase != ":memory:";});

                  if( common::algorithm::adjacent_find( common::algorithm::sort( persitent, order_qb), equal_gb))
                     code::raise::error( code::casual::invalid_configuration, "queue groups has to have unique queuebase paths within a configuration file");
               }

               // Check unique queues
               {
                  std::vector< std::string> queues;

                  for( auto& group : Manager::groups)
                     common::algorithm::transform( group.queues, queues, []( auto& queue){ return queue.name;});

                  if( common::algorithm::adjacent_find( common::algorithm::sort( queues)))
                     code::raise::error( code::casual::invalid_configuration, "queues has to be unique within a configuration file");
               }
               
            }
         } // queue

         namespace domain
         {
            Default& Default::operator += ( Default rhs)
            {
               environment += std::move( rhs.environment);
               service = coalesce( std::move( rhs.service), std::move( service));
               return *this;
            }
         } // domain
   
         Domain& Domain::operator += ( Domain rhs)
         {
            name = coalesce( std::move( rhs.name), std::move( name));
            note = coalesce( std::move( rhs.note), std::move( note));

            local::optional_add( rhs.defaults, defaults);

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


            // validate
            {
               local::validate::unique( groups, local::equal::name(), "groups has to have unique names");
               local::validate::unique( servers, local::equal::alias(), "servers has to have a unique alias, if defined");
               local::validate::unique( executables, local::equal::alias(), "executables has to have a unique alias, if defined");
            }

            if( ! defaults)
               return; // nothing to normalize

            
            auto normalize_entities = []( auto& entities, auto& defaults)
            {
               auto normalize_entity = [&defaults]( auto& entity)
               {
                  entity.instances = entity.instances.value_or( defaults.instances);
                  entity.memberships = entity.memberships.value_or( defaults.memberships);
                  entity.environment = entity.environment.value_or( defaults.environment);
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
                  service.timeout = service.timeout.value_or( defaults.timeout);
               };
               algorithm::for_each( services, normalize_service);
            }

         }

      } // user
   } // configuration
} // casual