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

#include "serviceframework/service/protocol.h"

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

            namespace service
            {
               auto state( const manager::State& state)
               {
                  return [&state]( common::service::invoke::Parameter&& parameter)
                  {
                     return serviceframework::service::user( 
                        serviceframework::service::protocol::deduce( std::move( parameter)), 
                        []( auto& state){ return transform::state( state);}, 
                        state);
                  };
               }

               namespace scale
               {
                  auto aliases( manager::State& state)
                  {
                     return [&state]( common::service::invoke::Parameter&& parameter)
                     {
                        auto protocol = serviceframework::service::protocol::deduce( std::move( parameter));
                        auto aliases = protocol.extract< std::vector< model::scale::Alias>>( "aliases");

                        return serviceframework::service::user( std::move( protocol), &handle::scale::aliases, state, std::move( aliases));
                     };
                  }     
               } // scale


               namespace restart
               {
                  auto aliases( manager::State& state)
                  {
                     return [&state]( common::service::invoke::Parameter&& parameter)
                     {
                        auto protocol = serviceframework::service::protocol::deduce( std::move( parameter));
                        auto aliases = protocol.extract< std::vector< model::restart::Alias>>( "aliases");

                        return serviceframework::service::user( std::move( protocol), &local::restart::aliases, state, std::move( aliases));
                     };
                  }

                  auto groups( manager::State& state)
                  {
                     return [&state]( common::service::invoke::Parameter&& parameter)
                     {
                        auto protocol = serviceframework::service::protocol::deduce( std::move( parameter));
                        auto groups = protocol.extract< std::vector< model::restart::Group>>( "groups");

                        return serviceframework::service::user( std::move( protocol), &local::restart::groups, state, std::move( groups));
                     };
                  }
               } // restart



               auto shutdown( manager::State& state)
               {
                  return [&state]( common::service::invoke::Parameter&& parameter)
                  {
                     return serviceframework::service::user( 
                        serviceframework::service::protocol::deduce( std::move( parameter)), 
                        &handle::shutdown, state);
                  };
               }

               namespace environment
               {
                  auto set( manager::State& state)
                  {
                     return [&state]( common::service::invoke::Parameter&& parameter)
                     {
                        auto protocol = serviceframework::service::protocol::deduce( std::move( parameter));

                        model::set::Environment environment;
                        protocol >> CASUAL_NAMED_VALUE( environment);

                        return serviceframework::service::user(
                           std::move( protocol),
                           &local::set::environment,
                           state, 
                           environment);
                     };
                  }
               } // environment

               namespace configuration
               {
                  auto get( const manager::State& state)
                  {
                     return [&state]( common::service::invoke::Parameter&& parameter)
                     {
                        auto get_configuration = []( auto& state)
                        {
                           return casual::configuration::model::transform( manager::configuration::get( state));
                        };

                        return serviceframework::service::user( 
                           serviceframework::service::protocol::deduce( std::move( parameter)),
                           get_configuration,
                           state);
                     };
                  }

                  auto post( manager::State& state)
                  {
                     return [&state]( common::service::invoke::Parameter&& parameter)
                     {
                        Trace trace{ "domain::manager::admin::local::service::configuration::post"};

                        auto protocol = serviceframework::service::protocol::deduce( std::move( parameter));
                        auto wanted = normalize( casual::configuration::model::transform( protocol.extract< casual::configuration::user::Domain>( "domain")));

                        auto post_configuration = []( auto& state, auto& wanted)
                        {
                           state.configuration.model = manager::configuration::get( state);
                           return manager::configuration::post( state, std::move( wanted));
                        };

                        return serviceframework::service::user( 
                           std::move( protocol),
                           post_configuration,
                           state, wanted);
                     };
                  }

                  auto put( manager::State& state)
                  {
                     return [&state]( common::service::invoke::Parameter&& parameter)
                     {
                        auto protocol = serviceframework::service::protocol::deduce( std::move( parameter));
                        auto updates = casual::configuration::model::transform( protocol.extract< casual::configuration::user::Domain>( "domain"));

                        auto post_configuration = []( auto& state, auto& updates)
                        {
                           state.configuration.model = manager::configuration::get( state);
                           return manager::configuration::post( state, normalize( state.configuration.model + std::move( updates)));
                        };

                        return serviceframework::service::user( 
                           std::move( protocol),
                           post_configuration,
                           state, updates);
                     };
                  }
               } // configuration

            } // service
         } // <unnamed>
      } // local

      common::server::Arguments services( manager::State& state)
      {
         return { {
               { service::name::state,
                  local::service::state( state),
                  common::service::transaction::Type::none,
                  common::service::category::admin
               },
               { service::name::scale::aliases,
                     local::service::scale::aliases( state),
                     common::service::transaction::Type::none,
                     common::service::category::admin
               },
               { service::name::restart::aliases,
                     local::service::restart::aliases( state),
                     common::service::transaction::Type::none,
                     common::service::category::admin
               },
               { service::name::restart::groups,
                     local::service::restart::groups( state),
                     common::service::transaction::Type::none,
                     common::service::category::admin
               },
               { service::name::shutdown,
                     local::service::shutdown( state),
                     common::service::transaction::Type::none,
                     common::service::category::admin
               },
               { service::name::configuration::get,
                     local::service::configuration::get( state),
                     common::service::transaction::Type::none,
                     common::service::category::admin
               },
               { service::name::configuration::post,
                     local::service::configuration::post( state),
                     common::service::transaction::Type::none,
                     common::service::category::admin
               },
               { service::name::configuration::put,
                     local::service::configuration::put( state),
                     common::service::transaction::Type::none,
                     common::service::category::admin
               },
               { service::name::environment::set,
                     local::service::environment::set( state),
                     common::service::transaction::Type::none,
                     common::service::category::admin
               },
               // deprecated
               { ".casual/domain/scale/instances",
                     local::service::scale::aliases( state),
                     common::service::transaction::Type::none,
                     common::service::category::deprecated
               },
               { ".casual/domain/restart/instances",
                     local::service::restart::aliases( state),
                     common::service::transaction::Type::none,
                     common::service::category::deprecated
               },
         }};
      }
   } // gateway::manager::admin
} // casual
