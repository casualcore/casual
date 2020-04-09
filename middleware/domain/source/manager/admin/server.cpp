//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/manager/admin/server.h"
#include "domain/manager/admin/model.h"
#include "domain/manager/handle.h"
#include "domain/manager/configuration.h"
#include "domain/transform.h"

#include "configuration/model.h"
#include "configuration/model/transform.h"

#include "serviceframework/service/protocol.h"

namespace casual
{
   using namespace common;


   namespace domain
   {

      namespace manager
      {
         namespace admin
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
                        const auto variables = casual::configuration::user::environment::transform( 
                           casual::configuration::user::environment::fetch( environment.variables));

                        auto update_environment = [&variables]( auto& entity)
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
                              []( auto& state){ return casual::domain::transform::state( state);}, 
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

                              std::vector< model::scale::Alias> aliases;
                              protocol >> CASUAL_NAMED_VALUE( aliases);

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
                              
                              std::vector< model::restart::Alias> aliases;
                              protocol >> CASUAL_NAMED_VALUE( aliases);

                              return serviceframework::service::user( std::move( protocol), &local::restart::aliases, state, std::move( aliases));
                           };
                        }

                        auto groups( manager::State& state)
                        {
                           return [&state]( common::service::invoke::Parameter&& parameter)
                           {
                              auto protocol = serviceframework::service::protocol::deduce( std::move( parameter));
                              
                              std::vector< model::restart::Group> groups;
                              protocol >> CASUAL_NAMED_VALUE( groups);

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
                              auto get_configuration = [&state]()
                              {
                                 return casual::configuration::model::transform( 
                                    casual::domain::manager::configuration::get( state));
                              };

                              return serviceframework::service::user( 
                                 serviceframework::service::protocol::deduce( std::move( parameter)),
                                 get_configuration);
                           };
                        }

                        auto put( manager::State& state)
                        {
                           return [&state]( common::service::invoke::Parameter&& parameter)
                           {
                              auto protocol = serviceframework::service::protocol::deduce( std::move( parameter));

                              auto model = [&]()
                              {
                                 casual::configuration::user::Domain domain;
                                 protocol >> CASUAL_NAMED_VALUE( domain);

                                 return casual::configuration::model::transform( std::move( domain));
                              }();


                              return serviceframework::service::user( 
                                 std::move( protocol),
                                 [&](){ return casual::domain::manager::configuration::put( state, std::move( model));});
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
         } // admin
      } // manager
   } // gateway
} // casual
