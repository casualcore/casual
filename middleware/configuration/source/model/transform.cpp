//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "configuration/model/transform.h"
#include "configuration/user/environment.h"
#include "configuration/common.h"

#include "common/chronology.h"

namespace casual
{
   using namespace common;
   namespace configuration::model
   {
      namespace local
      {
         namespace
         {

            template< typename T>
            T empty_if_null( std::optional< T> value)
            {
               if( value)
                  return std::move( value.value());
               return {};
            }

            namespace normalize
            {
               template< typename P>
               auto mutator( std::map< std::string, std::size_t>& state, P&& prospect)
               {
                  return [ &state, prospect = std::forward< P>( prospect)]( auto& value)
                  {
                     if( value.alias.empty())
                        value.alias = prospect( value);
                     
                     auto potentally_add_index = []( auto& state, auto& alias)
                     {
                        auto count = ++state[ alias];

                        if( count == 1)
                           return false;

                        alias = string::compose( alias, ".", count);
                        return true;
                     };

                     while( potentally_add_index( state, value.alias))
                        ; // no-op
                  };
               }
               void aliases( configuration::Model& model)
               {
                  {
                     auto state = std::map< std::string, std::size_t>{};
                     auto normalizer = normalize::mutator( state, []( auto& value){ return file::name::base( value.path);});
                     algorithm::for_each( model.domain.executables, normalizer);
                     algorithm::for_each( model.domain.servers, normalizer);
                  }

                  {
                     auto state = std::map< std::string, std::size_t>{};
                     auto forward_alias = normalize::mutator( state, []( auto&){ return "forwrad";});

                     algorithm::for_each( model.queue.forward.groups, [&forward_alias, &state]( auto& group)
                     {
                        forward_alias( group);
                        
                        // normalize the forwards
                        auto normalizer = normalize::mutator( state, []( auto& value){ return value.source;});
                        algorithm::for_each( group.services, normalizer);
                        algorithm::for_each( group.queues, normalizer);
                     });;
                  }

                  {
                     auto state = std::map< std::string, std::size_t>{};
                     
                     {
                        auto normalizer = normalize::mutator( state, []( auto& value)
                        { 
                           if( value.connect == decltype( value.connect)::reversed)
                              return "reverse.inbound";
                           return "inbound";
                        
                        });
                        algorithm::for_each( model.gateway.inbound.groups, normalizer);
                     }

                     {
                        auto normalizer = normalize::mutator( state, []( auto& value)
                        { 
                           if( value.connect == decltype( value.connect)::reversed)
                              return "reverse.outbound";
                           return "outbound";
                        
                        });
                        algorithm::for_each( model.gateway.outbound.groups, normalizer);
                     }
                  }
               }

               auto order()
               {
                  return [ order = platform::size::type{ 0}]( auto& value) mutable
                  {
                     value.order = order++;
                  };
               }
            } // normalize

            auto environment( const user::Environment& environment)
            {
               domain::Environment result;
               result.variables = user::environment::transform( user::environment::fetch( environment));
               return result;
            }

            auto transaction( const configuration::user::Domain& domain)
            {
               Trace trace{ "configuration::model::local::transaction"};

               transaction::Model result;

               if( ! domain.transaction)
                  return result;

               auto& transaction = domain.transaction.value();

               result.log = transaction.log;

               result.resources = common::algorithm::transform( transaction.resources, []( const auto& resource)
               {
                  transaction::Resource result;

                  result.name = resource.name;
                  result.key = resource.key.value_or( "");
                  result.instances = resource.instances.value_or( 0);
                  result.note = resource.note.value_or( "");
                  result.openinfo = resource.openinfo.value_or( "");
                  result.closeinfo = resource.closeinfo.value_or( "");

                  return result;
               });

               return result;
            }

            auto domain( const configuration::user::Domain& domain)
            {
               Trace trace{ "configuration::model::local::domain"};

               domain::Model result;
               
               result.name = domain.name;

               if( domain.environment)
                  result.environment = local::environment( domain.environment.value());

               // groups
               result.groups = common::algorithm::transform( domain.groups, []( auto& group)
               {
                  domain::Group result;
                  result.name = group.name;
                  result.note = group.note.value_or( "");
                  result.resources = group.resources.value_or( result.resources);
                  result.dependencies = group.dependencies.value_or( result.dependencies);
                  return result;
               });

               auto assign_executable = []( auto& entity, auto& result)
               {
                  result.alias = entity.alias.value_or( "");
                  result.arguments = entity.arguments.value_or( result.arguments);
                  result.instances = entity.instances.value_or( result.instances);
                  result.note = entity.note.value_or( "");
                  result.path = entity.path;
                  result.lifetime.restart = entity.restart.value_or( result.lifetime.restart);
                  result.memberships = entity.memberships.value_or( result.memberships);

                  // TOOD performance: worst case we'd read env-files a lot of times...
                  if( entity.environment)
                     result.environment = local::environment( entity.environment.value());
               };

               result.executables = common::algorithm::transform( domain.executables, [&]( auto& value)
               {
                  domain::Executable result;
                  assign_executable( value, result);
                  return result;
               });

               result.servers = common::algorithm::transform( domain.servers, [&]( auto& value)
               {
                  domain::Server result;
                  assign_executable( value, result);
                  if( value.resources)
                     result.resources = value.resources.value();
                  if( value.restrictions)
                     result.restrictions = value.restrictions.value();

                  return result;
               });

               return result;
            }


            auto service( const configuration::user::Domain& domain)
            {
               Trace trace{ "configuration::model::local::service"};

               service::Model result;

               if( domain.defaults && domain.defaults.value().service)
                  result.timeout = common::chronology::from::string( domain.defaults.value().service.value().timeout);

               result.services = common::algorithm::transform( domain.services, []( const auto& service)
               {
                  service::Service result;

                  result.name = service.name;
                  result.routes = service.routes.value_or( result.routes);
                  if( service.timeout)
                     result.timeout = common::chronology::from::string( service.timeout.value());

                  return result;
               });

               return result;
            }

            auto gateway( const configuration::user::Domain& domain)
            {
               Trace trace{ "configuration::model::local::gateway"};

               gateway::Model result;

               if( ! domain.gateway)
                  return result;

               auto& gateway = domain.gateway.value();

               // first we take care of deprecated stuff

               if( ! gateway.listeners.empty())
               {
                  log::line( log::category::warning, code::casual::invalid_configuration, " domain.gateway.listeners are deprecated - use domain.gateway.inbounds");

                  gateway::inbound::Group group;
                  group.connect = decltype( group.connect)::regular;
                  group.connections = common::algorithm::transform( gateway.listeners, []( const auto& listener)
                  {
                     gateway::inbound::Connection result;
                     result.note = listener.note.value_or( "");
                     result.address = listener.address;
                     return result;
                  });


                  group.limit = algorithm::accumulate( gateway.listeners, gateway::inbound::Limit{}, [&]( auto current, auto& listener)
                  {
                     if( ! listener.limit)
                        return current;

                     auto size = listener.limit.value().size.value_or( 0);
                     auto messages = listener.limit.value().messages.value_or( 0);

                     if( size > 0 && current.size > size)
                        current.size = size;

                     if( messages > 0 && current.messages > messages)
                        current.messages = messages;

                     return current;
                  });

                  result.inbound.groups.push_back( std::move( group));
               }

               if( ! gateway.connections.empty())
               {
                  log::line( log::category::warning, code::casual::invalid_configuration, " domain.gateway.connections are deprecated - use domain.gateway.outbounds");

                  gateway::outbound::Group group;
                  group.connect = decltype( group.connect)::regular;
                  group.connections = common::algorithm::transform( gateway.connections, []( const auto& value)
                  {
                     gateway::outbound::Connection result;

                     result.note = value.note.value_or( "");
                     result.address = value.address;
                     
                     if( value.services)
                        result.services = value.services.value();
                     if( value.queues)
                        result.queues = value.queues.value();

                     return result;
                  });
                  result.outbound.groups.push_back( std::move( group));
               }

               auto append_inbounds = []( auto& source, auto& target, auto connect)
               { 
                  if( ! source)
                     return;

                  algorithm::transform( source.value().groups, std::back_inserter( target), [connect]( auto& source)
                  {
                     gateway::inbound::Group result;
                     result.alias = source.alias.value_or( "");
                     result.note = source.note.value_or( "");
                     result.connect = connect;

                     if( source.limit)
                     {
                        result.limit.size = source.limit.value().size.value_or( result.limit.size);
                        result.limit.messages = source.limit.value().messages.value_or( result.limit.messages);
                     }

                     result.connections = algorithm::transform( source.connections, []( auto& connection)
                     {
                        gateway::inbound::Connection result;
                        result.note = connection.note.value_or( "");
                        result.address = connection.address;
                        return result;
                     });

                     return result;
                  });
               };

               auto append_outbounds = []( auto& source, auto& target, auto connect)
               { 
                  if( ! source)
                     return;

                  algorithm::transform( source.value().groups, std::back_inserter( target), [connect]( auto& source)
                  {
                     gateway::outbound::Group result;
                     result.alias = source.alias.value_or( "");
                     result.note = source.note.value_or( "");
                     result.connect = connect;
                     result.connections = algorithm::transform( source.connections, []( auto& connection)
                     {
                        gateway::outbound::Connection result;
                        result.note = connection.note.value_or( "");
                        result.address = connection.address;
                        result.services = local::empty_if_null( std::move( connection.services));
                        result.queues = local::empty_if_null( std::move( connection.queues));
                        return result;
                     });

                     return result;
                  });
               };

               append_inbounds( gateway.inbound, result.inbound.groups, configuration::model::gateway::connect::Semantic::regular);
               append_outbounds( gateway.outbound, result.outbound.groups, configuration::model::gateway::connect::Semantic::regular);

               if( gateway.reverse)
               {
                  append_inbounds( gateway.reverse.value().inbound, result.inbound.groups, configuration::model::gateway::connect::Semantic::reversed);
                  append_outbounds( gateway.reverse.value().outbound, result.outbound.groups, configuration::model::gateway::connect::Semantic::reversed);
               }

               // make sure we keep track of the order.
               algorithm::for_each( result.outbound.groups, normalize::order());

               return result;
            }

            auto queue( const configuration::user::Domain& domain)
            {
               Trace trace{ "configuration::model::local::queue"};

               queue::Model result;

               if( ! domain.queue)
                  return result;

               auto& source = domain.queue.value();

               if( source.groups)
               {
                  result.groups = common::algorithm::transform( source.groups.value(), []( auto& group)
                  {
                     queue::Group result;

                     result.alias = group.alias.value_or( "");
                     result.note = group.note.value_or( "");
                     result.queuebase = group.queuebase.value_or( "");

                     common::algorithm::transform( group.queues, result.queues, []( auto& queue)
                     {
                        model::queue::Queue result;

                        result.name = queue.name;
                        result.note = queue.note.value_or("");
                        if( queue.retry)
                        {
                           auto& retry = queue.retry.value();
                           if( retry.count)
                              result.retry.count = retry.count.value();
                           if( retry.delay)
                              result.retry.delay = common::chronology::from::string( retry.delay.value());
                        }

                        return result;
                     });
                     return result;
                  });
               }
               
               
               if( source.forward && source.forward.value().groups)
               {
                  result.forward.groups = algorithm::transform( source.forward.value().groups.value(), []( auto& group)
                  {
                     queue::forward::Group result;

                     auto set_base_forward = []( auto& source, auto& target)
                     {
                        target.alias = source.alias.value_or( "");
                        target.source = source.source;
                        if( source.instances)
                           target.instances = source.instances.value();

                        target.note = source.note.value_or( "");
                     };

                     if( group.services)
                     {
                        result.services = algorithm::transform( group.services.value(), [&set_base_forward]( auto& service)
                        {
                           configuration::model::queue::forward::Service result;

                           set_base_forward( service, result);
                           result.target.service = service.target.service;

                           if( service.reply)
                           {
                              configuration::model::queue::forward::Service::Reply reply;
                              reply.queue = service.reply.value().queue;

                              if( service.reply.value().delay)
                                 reply.delay =  common::chronology::from::string( service.reply.value().delay.value());
            
                              result.reply = std::move( reply);
                           }

                           return result;
                        });
                     }

                     if( group.queues)
                     {
                        result.queues = algorithm::transform( group.queues.value(), [&set_base_forward]( auto& queue)
                        {
                           configuration::model::queue::forward::Queue result;

                           set_base_forward( queue, result);

                           result.target.queue = queue.target.queue;
                           if( queue.target.delay)
                              result.target.delay = common::chronology::from::string( queue.target.delay.value());

                           return result;
                        });
                     }
                     
                     return result;
                  });
               }
         
               return result;
            }

            namespace model
            {
               namespace detail
               {
                  template< typename T>
                  auto empty( const T& value, traits::priority::tag< 0>) -> decltype( value == 0)
                  {
                     return value == 0;
                  }

                  template< typename T>
                  auto empty( const T& value, traits::priority::tag< 1>) -> decltype( value.empty())
                  {
                     return value.empty();
                  }
               } // detail

               template< typename T>
               auto null_if_empty( T&& value)
               {
                  using optional_t = std::optional< std::decay_t< T>>;

                  if( detail::empty( value, traits::priority::tag< 1>{}))
                     return optional_t{};

                  return optional_t{ std::forward< T>( value)};
               };

               auto domain( const configuration::Model& model)
               {
                  Trace trace{ "configuration::model::local::model::domain"};

                  configuration::user::Domain result;

                  result.name = model.domain.name;

                  if( model.service.timeout > platform::time::unit::zero())
                     result.defaults.emplace().service.emplace().timeout = chronology::to::string( model.service.timeout);

                  if( ! model.domain.environment.variables.empty())
                     result.environment.emplace().variables = configuration::user::environment::transform( model.domain.environment.variables);

                  auto assign_executable = []( auto& source, auto& target)
                  {
                     target.alias = source.alias;
                     target.path = source.path;
                     target.memberships = source.memberships;
                     target.restart = source.lifetime.restart;
                     target.arguments = source.arguments;
                     target.instances = source.instances;
                     target.note = null_if_empty( source.note);

                     
                     if( ! source.environment.variables.empty())
                     {
                        configuration::user::Environment environment;
                        environment.variables = user::environment::transform( source.environment.variables);
                        target.environment = std::move( environment);
                     }
                        
                  };

                  result.servers = algorithm::transform( model.domain.servers, [&]( auto& value)
                  {
                     configuration::user::domain::Server result;
                     assign_executable( value, result);
                     
                     result.resources = null_if_empty( value.resources);
                     result.restrictions = null_if_empty( value.restrictions);

                     return result;
                  });
               
                  result.executables = algorithm::transform( model.domain.executables, [&]( auto& value)
                  {
                     configuration::user::domain::Executable result;
                     assign_executable( value, result);
                     return result;
                  });

                  result.groups = algorithm::transform( model.domain.groups, []( auto& value)
                  {
                     configuration::user::domain::Group result;
                     result.name = value.name;
                     result.note = null_if_empty( value.note);
                     result.dependencies = null_if_empty( value.dependencies);
                     result.resources = null_if_empty( value.resources);

                     return result;
                  });

                  return result;
               }

               auto service( const configuration::Model& model)
               {
                  Trace trace{ "configuration::model::local::model::service"};

                  return algorithm::transform( model.service.services, []( auto& service)
                  {
                     user::Service result;

                     result.routes = null_if_empty( service.routes);
                     result.name = service.name;
                     result.timeout = null_if_empty( chronology::to::string( service.timeout));

                     return result;
                  });

               }

               auto transaction( const configuration::Model& model)
               {
                  Trace trace{ "configuration::model::local::model::transaction"};

                  user::transaction::Manager result;

                  result.log = model.transaction.log;
                  result.resources = algorithm::transform( model.transaction.resources, []( auto& value)
                  {
                     user::transaction::Resource result;

                     result.name = value.name;
                     result.key = null_if_empty( value.key);
                     result.openinfo = null_if_empty( value.openinfo);
                     result.closeinfo = null_if_empty( value.closeinfo);
                     result.note  = null_if_empty( value.note);
                     result.instances = value.instances;

                     return result;
                  });

                  return result;
               }

               auto gateway( configuration::model::gateway::Model model)
               {
                  Trace trace{ "configuration::model::local::model::gateway"};

                  user::gateway::Manager result;

                  auto transform_inbound = []( auto& value) 
                  {
                     configuration::user::gateway::inbound::Group result;
                     result.alias = value.alias;
                     result.note = null_if_empty( value.note);
                     
                     if( value.limit.size > 0 || value.limit.messages > 0)
                     {
                        user::gateway::inbound::Limit limit;
                        limit.messages = null_if_empty( value.limit.messages);
                        limit.size = null_if_empty( value.limit.size);
                        result.limit = std::move( limit);
                     };

                     result.connections = algorithm::transform( value.connections, []( auto& value)
                     {
                        configuration::user::gateway::inbound::Connection result;
                        result.address = value.address;
                        result.note = null_if_empty( value.note);
                        return result;
                     });
                     return result;
                  };

                  auto transform_outbound = []( auto& value) 
                  {
                     configuration::user::gateway::outbound::Group result;
                     result.alias = value.alias;
                     result.note = null_if_empty( value.note);
                     result.connections = algorithm::transform( value.connections, []( auto& value)
                     {
                        configuration::user::gateway::outbound::Connection result;
                        result.address = value.address;
                        result.note = null_if_empty( value.note);
                        result.services = null_if_empty( value.services);
                        result.queues = null_if_empty( value.queues);
                        return result;
                     });
                     return result;
                  };

                  auto is_reversed = []( auto& value){ return value.connect == decltype( value.connect)::reversed;};
                  
                  auto reverse = configuration::user::gateway::Reverse{};


                  // inbounds
                  {
                     auto [ reversed, regular] = algorithm::stable_partition( model.inbound.groups, is_reversed);

                     if( reversed)
                     {
                        configuration::user::gateway::Inbound inbound;
                        inbound.groups = algorithm::transform( reversed, transform_inbound);
                        reverse.inbound = std::move( inbound);
                     }

                     if( regular)
                     {
                        configuration::user::gateway::Inbound inbound;
                        inbound.groups = algorithm::transform( regular, transform_inbound);
                        result.inbound = std::move( inbound);
                     }
                        
                  }

                  // outbounds
                  {
                     auto less_order = []( auto& lhs, auto& rhs){ return lhs.order < rhs.order;};

                     auto [ reversed, regular] = algorithm::stable_partition( model.outbound.groups, is_reversed);

                     if( reversed)
                     {
                        configuration::user::gateway::Outbound outbound;
                        outbound.groups = algorithm::transform( algorithm::sort( reversed, less_order), transform_outbound);
                        reverse.outbound = std::move( outbound);
                     }

                     if( regular)
                     {
                        configuration::user::gateway::Outbound outbound;
                        outbound.groups = algorithm::transform( algorithm::sort( regular, less_order), transform_outbound);
                        result.outbound = std::move( outbound);
                     }
                  }

                  if( reverse.inbound || reverse.outbound)
                     result.reverse = std::move( reverse);
                  
                  return result;
               }

               auto queue( const configuration::Model& model)
               {
                  Trace trace{ "configuration::model::local::model::queue"};

                  user::queue::Manager result;
                  result.note  = null_if_empty( model.queue.note);

                  result.groups = null_if_empty( algorithm::transform( model.queue.groups, []( auto& value)
                  {
                     user::queue::Group result;
                     result.alias = null_if_empty( value.alias);
                     result.queuebase = null_if_empty( value.queuebase);

                     result.queues = algorithm::transform( value.queues, []( auto& value)
                     {
                        user::queue::Queue result;
                        result.name = value.name;
                        if( ! value.retry.empty())
                        {
                           user::queue::Queue::Retry retry;
                           retry.count = null_if_empty( value.retry.count);
                           retry.delay = null_if_empty( chronology::to::string( value.retry.delay));
                           result.retry = std::move( retry);
                        }
                        
                        result.note  = null_if_empty( value.note);
                        return result;
                     });
                     result.note  = null_if_empty( value.note);
                     return result;
                  }));

                  if( ! model.queue.forward.groups.empty())
                  {
                     user::queue::Forward forward;

                     forward.groups = null_if_empty( algorithm::transform( model.queue.forward.groups, []( auto& group)
                     {
                        user::queue::forward::Group result;
                        result.alias = group.alias;
                        result.note = null_if_empty( group.note);

                        result.services = null_if_empty( algorithm::transform( group.services, []( auto& service)
                        {
                           user::queue::forward::Service result;
                           result.alias = service.alias;
                           result.instances = service.instances;
                           result.note = null_if_empty( service.note);
                           result.source = service.source;
                           result.target.service = service.target.service;

                           if( service.reply)
                           {
                              user::queue::forward::Service::Reply reply;
                              reply.queue = service.reply.value().queue;
                              reply.delay = chronology::to::string( service.reply.value().delay);
                              result.reply = std::move( reply);
                           }

                           return result;
                        }));

                        result.queues = null_if_empty( algorithm::transform( group.queues, []( auto& queue)
                        {
                           user::queue::forward::Queue result;
                           result.alias = queue.alias;
                           result.instances = queue.instances;
                           result.note = null_if_empty( queue.note);
                           result.source = queue.source;
                           result.target.queue = queue.target.queue;
                           result.target.delay = chronology::to::string( queue.target.delay);

                           return result;
                        }));


                        return result;
                     }));
                     result.forward = std::move( forward);
                  }
                  
                  return result;
               }
            } // model

         } // <unnamed>
      } // local

      configuration::Model transform( user::Domain domain)
      {
         Trace trace{ "configuration::model::transform domain"};
         log::line( verbose::log, "domain: ", domain);

         configuration::Model result;
         result.domain = local::domain( domain);
         result.service = local::service( domain);
         result.transaction = local::transaction( domain);
         result.gateway = local::gateway( domain);
         result.queue = local::queue( domain);

         // make sure to normalize all aliases
         local::normalize::aliases( result);

         return result;
      }

      user::Domain transform( const configuration::Model& domain)
      {
         Trace trace{ "configuration::model::transform model"};

         auto result = local::model::domain( domain);
         result.services = local::model::service( domain);
         result.transaction = local::model::transaction( domain);
         result.gateway = local::model::gateway( domain.gateway);
         result.queue = local::model::queue( domain);

         return result;
      }

   } // configuration::model
} // casual
