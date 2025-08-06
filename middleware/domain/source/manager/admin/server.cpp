//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/manager/admin/server.h"
#include "domain/manager/admin/model.h"
#include "domain/manager/handle.h"
#include "domain/manager/configuration.h"
#include "domain/manager/transform.h"

#include "configuration/model.h"
#include "configuration/model/transform.h"
#include "configuration/message.h"

#include "casual/manager/service/protocol.h"

namespace casual
{
   using namespace common;

   namespace domain::manager::admin
   {

      namespace local
      {
         namespace
         {
            namespace restart
            {
               auto transform_name = []( auto& value)
               {
                  return value.name;
               };

               auto aliases( manager::State& state, std::vector< model::restart::Alias> aliases)
               {
                  Trace trace{ "domain::manager::admin::local::restart::instances"};
                  return handle::restart::aliases( state, algorithm::transform( aliases, transform_name));                       
               }

               auto groups( manager::State& state, std::vector< model::restart::Group> groups)
               {
                  Trace trace{ "domain::manager::admin::local::restart::instances"};
                  return handle::restart::groups( state, algorithm::transform( groups, transform_name));                       
               }
            } // restart

            namespace set
            {
               auto environment( manager::State& state, const model::set::Environment& environment)
               {

                  auto update_environment = [&variables = environment.variables]( auto& entity)
                  {
                     auto update_variable = [&entity]( const auto& variable)
                     {
                        auto is_name = [&variable]( auto& value)
                        {
                           return variable.name() == value.name();
                        };

                        if( auto found = algorithm::find_if( entity.environment.variables, is_name))
                           *found = variable;
                        else
                           entity.environment.variables.push_back( variable);
                     };

                     algorithm::for_each( variables, update_variable);

                     return entity.alias;
                  };

                  // if empty, we update all 'entities'
                  if( environment.aliases.empty())
                  {
                     auto result = algorithm::transform( state.servers, update_environment);
                     algorithm::transform( state.executables, result, update_environment);
                     return result;
                  }

                  // else, we correlate aliases

                  std::vector< std::string> result;

                  auto find_and_update = [&]( auto& entity)
                  {
                     auto is_alias = [&entity]( auto& alias){ return alias == entity.alias;};

                     if( auto found = algorithm::find_if( environment.aliases, is_alias))
                        result.push_back( update_environment( entity));
                  };

                  algorithm::for_each( state.servers, find_and_update);
                  algorithm::for_each( state.executables, find_and_update);

                  return result;
               };
            } // set

            namespace unset
            {
               auto environment( manager::State& state, const model::unset::Environment& environment)
               {
                  auto update_environment = [&variables = environment.variables]( auto& entity)
                  {
                     auto update_variable = [&entity]( const auto& variable)
                     {
                        auto is_name = [&variable]( auto& value)
                        {
                           return variable == value.name();
                        };

                        algorithm::container::trim( entity.environment.variables, algorithm::remove_if( entity.environment.variables, is_name));
                     };

                     algorithm::for_each( variables, update_variable);

                     return entity.alias;
                  };

                  // if empty, we update all 'entities'
                  if( environment.aliases.empty())
                  {
                     auto result = algorithm::transform( state.servers, update_environment);
                     algorithm::transform( state.executables, result, update_environment);
                     return result;
                  }

                  // else, we correlate aliases

                  std::vector< std::string> result;

                  auto find_and_update = [&]( auto& entity)
                  {
                     auto is_alias = [&entity]( auto& alias){ return alias == entity.alias;};

                     if( auto found = algorithm::find_if( environment.aliases, is_alias))
                        result.push_back( update_environment( entity));
                  };

                  algorithm::for_each( state.servers, find_and_update);
                  algorithm::for_each( state.executables, find_and_update);

                  return result;
               };
            }

            namespace service
            {
               auto state( const manager::State& state)
               {
                  return [&state]( casual::manager::service::invoke::Parameter&& parameter)
                  {
                     return casual::manager::service::protocol::dispatch(
                        std::move( parameter), 
                        []( auto& state){ return transform::state( state);}, 
                        state);
                  };
               }

               namespace scale
               {
                  auto aliases( manager::State& state)
                  {
                     return [&state]( casual::manager::service::invoke::Parameter&& parameter)
                     {
                        auto protocol = casual::manager::service::protocol::deduce( std::move( parameter));
                        auto aliases = protocol.extract< std::vector< model::scale::Alias>>( "aliases");

                        state::scale::Instances instances;

                        for( auto& alias : aliases)
                        {
                           if( auto found = state.server( alias.name))
                              instances.servers.push_back( { .id = found->id, .instances = alias.instances});
                           else if( auto found = state.executable( alias.name))
                              instances.executables.push_back( { .id = found->id, .instances = alias.instances});
                        }




                        return casual::manager::service::protocol::dispatch( std::move( protocol), &handle::scale::aliases, state, std::move( instances));
                     };
                  }     
               } // scale


               namespace restart
               {
                  auto aliases( manager::State& state)
                  {
                     return [&state]( casual::manager::service::invoke::Parameter&& parameter)
                     {
                        auto protocol = casual::manager::service::protocol::deduce( std::move( parameter));
                        auto aliases = protocol.extract< std::vector< model::restart::Alias>>( "aliases");

                        return casual::manager::service::protocol::dispatch( std::move( protocol), &local::restart::aliases, state, std::move( aliases));
                     };
                  }

                  auto groups( manager::State& state)
                  {
                     return [&state]( casual::manager::service::invoke::Parameter&& parameter)
                     {
                        auto protocol = casual::manager::service::protocol::deduce( std::move( parameter));
                        auto groups = protocol.extract< std::vector< model::restart::Group>>( "groups");

                        return casual::manager::service::protocol::dispatch( std::move( protocol), &local::restart::groups, state, std::move( groups));
                     };
                  }
               } // restart



               auto shutdown( manager::State& state)
               {
                  return [&state]( casual::manager::service::invoke::Parameter&& parameter)
                  {
                     return casual::manager::service::protocol::dispatch( 
                        casual::manager::service::protocol::deduce( std::move( parameter)), 
                        &handle::shutdown, state);
                  };
               }

               namespace environment
               {
                  auto set( manager::State& state)
                  {
                     return [&state]( casual::manager::service::invoke::Parameter&& parameter)
                     {
                        auto protocol = casual::manager::service::protocol::deduce( std::move( parameter));

                        model::set::Environment environment;
                        protocol >> CASUAL_NAMED_VALUE( environment);

                        return casual::manager::service::protocol::dispatch(
                           std::move( protocol),
                           &local::set::environment,
                           state, 
                           environment);
                     };
                  }

                  auto unset( manager::State& state)
                  {
                     return [&state]( casual::manager::service::invoke::Parameter&& parameter)
                     {
                        auto protocol = casual::manager::service::protocol::deduce( std::move( parameter));

                        model::unset::Environment environment;
                        protocol >> CASUAL_NAMED_VALUE( environment);

                        return casual::manager::service::protocol::dispatch(
                           std::move( protocol),
                           &local::unset::environment,
                           state, 
                           environment);
                     };
                  }
               } // environment

               namespace configuration
               {
                  auto get( manager::State& state)
                  {
                     return [&state]( casual::manager::service::invoke::Parameter&& parameter)
                     {
                        auto get_configuration = []( auto& state)
                        {
                           return casual::configuration::model::transform( manager::configuration::get( state));
                        };

                        return casual::manager::service::protocol::dispatch( 
                           casual::manager::service::protocol::deduce( std::move( parameter)),
                           get_configuration,
                           state);
                     };
                  }

                  auto post( manager::State& state)
                  {
                     return [&state]( casual::manager::service::invoke::Parameter&& parameter)
                     {
                        Trace trace{ "domain::manager::admin::local::service::configuration::post"};

                        auto protocol = casual::manager::service::protocol::deduce( std::move( parameter));
                        auto wanted = normalize( casual::configuration::model::transform( protocol.extract< casual::configuration::user::Model>()));

                        auto post_configuration = []( auto& state, auto& wanted)
                        {
                           state.configuration.model = manager::configuration::get( state);
                           return manager::configuration::post( state, std::move( wanted));
                        };

                        return casual::manager::service::protocol::dispatch( 
                           std::move( protocol),
                           post_configuration,
                           state, wanted);
                     };
                  }

                  auto put( manager::State& state)
                  {
                     return [&state]( casual::manager::service::invoke::Parameter&& parameter)
                     {
                        auto protocol = casual::manager::service::protocol::deduce( std::move( parameter));
                        auto updates = casual::configuration::model::transform( protocol.extract< casual::configuration::user::Model>());

                        auto post_configuration = []( auto& state, auto& updates)
                        {
                           state.configuration.model = manager::configuration::get( state);
                           return manager::configuration::post( state, normalize( state.configuration.model + std::move( updates)));
                        };

                        return casual::manager::service::protocol::dispatch( 
                           std::move( protocol),
                           post_configuration,
                           state, updates);
                     };
                  }
               } // configuration

            } // service
         } // <unnamed>
      } // local

      std::vector< casual::manager::Service> services( manager::State& state)
      {
         return { 
               casual::manager::sequential::Service{ .name = std::string{ service::name::state},
                  .function = local::service::state( state),
                  .visibility = common::service::visibility::Type::undiscoverable,
                  .category = std::string{ common::service::category::admin}
               },
               casual::manager::sequential::Service{ .name = std::string{ service::name::scale::aliases},
                  .function = local::service::scale::aliases( state),
                  .visibility = common::service::visibility::Type::undiscoverable,
                  .category = std::string{ common::service::category::admin}
               },
               casual::manager::sequential::Service{ .name = std::string{ service::name::restart::aliases},
                  .function = local::service::restart::aliases( state),
                  .visibility = common::service::visibility::Type::undiscoverable,
                  .category = std::string{ common::service::category::admin}
               },
               casual::manager::sequential::Service{ .name = std::string{ service::name::restart::groups},
                  .function = local::service::restart::groups( state),
                  .visibility = common::service::visibility::Type::undiscoverable,
                  .category = std::string{ common::service::category::admin}
               },
               casual::manager::sequential::Service{ .name = std::string{ service::name::shutdown},
                  .function = local::service::shutdown( state),
                  .visibility = common::service::visibility::Type::undiscoverable,
                  .category = std::string{ common::service::category::admin}
               },
               casual::manager::sequential::Service{ .name = std::string{ service::name::configuration::get},
                  .function = local::service::configuration::get( state),
                  .visibility = common::service::visibility::Type::undiscoverable,
                  .category = std::string{ common::service::category::admin}
               },
               casual::manager::sequential::Service{ .name = std::string{ service::name::configuration::post},
                  .function = local::service::configuration::post( state),
                  .visibility = common::service::visibility::Type::undiscoverable,
                  .category = std::string{ common::service::category::admin}
               },
               casual::manager::sequential::Service{ .name = std::string{ service::name::configuration::put},
                  .function = local::service::configuration::put( state),
                  .visibility = common::service::visibility::Type::undiscoverable,
                  .category = std::string{ common::service::category::admin}
               },
               casual::manager::sequential::Service{ .name = std::string{ service::name::environment::set},
                  .function = local::service::environment::set( state),
                  .visibility = common::service::visibility::Type::undiscoverable,
                  .category = std::string{ common::service::category::admin}
               },
               casual::manager::sequential::Service{ .name = std::string{ service::name::environment::unset},
                  .function = local::service::environment::unset( state),
                  .visibility = common::service::visibility::Type::undiscoverable,
                  .category = std::string{ common::service::category::admin}
               },
               // deprecated
               casual::manager::sequential::Service{ .name = ".casual/domain/scale/instances",
                  .function = local::service::scale::aliases( state),
                  .visibility = common::service::visibility::Type::undiscoverable,
                  .category = std::string{ common::service::category::deprecated}
               },
               casual::manager::sequential::Service{ .name = ".casual/domain/restart/instances",
                  .function = local::service::restart::aliases( state),
                  .visibility = common::service::visibility::Type::undiscoverable,
                  .category = std::string{ common::service::category::deprecated}
               },
         };
      }

      void Policy::send_ack( const common::message::service::call::ACK& ack)
      {
         Trace trace{ "domain::manager::admin::Policy::send_ack"};

         // we just push it to our own inbound device, and handle it later
         communication::ipc::inbound::device().push( ack);
      }

      void Policy::initialize( const casual::manager::service::context::State& services)
      {
         Trace trace{ "domain::manager::admin::Policy::initialize"};

         // no-op
      }

   } // gateway::manager::admin
} // casual
