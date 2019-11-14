//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/manager/admin/server.h"
#include "domain/manager/admin/model.h"
#include "domain/manager/handle.h"
#include "domain/manager/persistent.h"
#include "domain/manager/configuration.h"
#include "domain/transform.h"


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
                  namespace scale
                  {
                     std::vector< model::scale::Instances> instances( manager::State& state, std::vector< model::scale::Instances> instances)
                     {
                        std::vector< model::scale::Instances> result;

                        auto scale_entities = [&]( auto& instance, auto& entities)
                        {
                           auto found = algorithm::find_if( entities, [&instance]( auto& e){
                              return e.alias == instance.alias;
                           });

                           if( found)
                           {
                              found->scale( instance.instances);
                              handle::scale::instances( state, *found);

                              return true;
                           }
                           return false;
                        };

                        for( auto& instance : instances)
                        {
                           if( scale_entities( instance, state.servers) ||
                                 scale_entities( instance, state.executables))
                           {
                              result.push_back( std::move( instance));
                           }
                        }

                        return result;
                     }

                  } // scale

                  namespace restart
                  {
                     std::vector< model::restart::Result> instances( manager::State& state, std::vector< model::restart::Instances> instances)
                     {
                        Trace trace{ "domain::manager::admin::local::restart::instances"};

                        auto result = handle::restart::instances( state, algorithm::transform( instances, []( auto& i){ return std::move( i.alias);}));

                        auto transform = []( auto& v)
                        {
                           model::restart::Result result;
                           result.alias = std::move( v.alias);
                           result.pids = std::move( v.pids);
                           result.task = v.task;
                           return result;
                        };

                        return algorithm::transform( result, transform);                        
                     }
                  } // restart

                  namespace set
                  {
                     auto environment( manager::State& state, const model::set::Environment& environment)
                     {

                        // collect all relevant processes
                        auto processes = [&]()
                        {
                           std::vector< state::Process*> result;

                           if( environment.aliases.empty())
                           {
                              algorithm::transform( state.servers, result, []( auto& process){ return &process;});
                              algorithm::transform( state.executables, result, []( auto& process){ return &process;});
                           }
                           else 
                           {
                              auto find_alias = [&]( auto& alias) -> state::Process*
                              {
                                 auto equal_alias = [&alias]( auto& p){ return p.alias == alias;};
                                 {
                                    auto found = algorithm::find_if( state.servers, equal_alias);
                                    if( found)
                                       return &( *found);
                                 }
                                 {
                                    auto found = algorithm::find_if( state.executables, equal_alias);
                                    if( found)
                                       return &( *found);
                                 }
                                 return nullptr;
                              };

                              algorithm::transform( environment.aliases, result, find_alias);
                              algorithm::trim( result, algorithm::remove( result, nullptr));
                           }

                           return result;
                        }();

                        const auto variables = transform::environment::variables( environment.variables);

                        auto update_variables = [&]( auto process)
                        {

                           // replace or add the variable
                           auto update_variable = [&]( auto& variable)
                           {
                              auto found = algorithm::find_if( process->environment.variables, [&]( auto& v)
                              { 
                                 auto equal_variable = []( auto& lhs, auto& rhs)
                                 {
                                    return lhs.name() == rhs.name();
                                 };

                                 return equal_variable( v, variable);
                              });

                              if( found)
                                 *found = variable;
                              else 
                                 process->environment.variables.push_back( variable);
                           };

                           algorithm::for_each( variables, update_variable);

                           return process->alias;
                        };

                        return algorithm::transform( processes, update_variables);
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

                     auto scale( manager::State& state)
                     {
                        return [&state]( common::service::invoke::Parameter&& parameter)
                        {
                           auto protocol = serviceframework::service::protocol::deduce( std::move( parameter));

                           std::vector< model::scale::Instances> instances;
                           protocol >> CASUAL_NAMED_VALUE( instances);

                           return serviceframework::service::user( std::move( protocol), &scale::instances, state, std::move( instances));
                        };
                     }

                     auto restart( manager::State& state)
                     {
                        return [&state]( common::service::invoke::Parameter&& parameter)
                        {
                           auto protocol = serviceframework::service::protocol::deduce( std::move( parameter));
                           
                           std::vector< model::restart::Instances> instances;
                           protocol >> CASUAL_NAMED_VALUE( instances);

                           return serviceframework::service::user( std::move( protocol), &restart::instances, state, std::move( instances));
                        };
                     }

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
                        auto persist( const manager::State& state)
                        {
                           return [&state]( common::service::invoke::Parameter&& parameter)
                           {
                              return serviceframework::service::user( 
                                 serviceframework::service::protocol::deduce( std::move( parameter)),
                                 []( auto& state){ return persistent::state::save( state);},
                                 state);
                           };
                        }

                        auto get( const manager::State& state)
                        {
                           return [&state]( common::service::invoke::Parameter&& parameter)
                           {
                              return serviceframework::service::user( 
                                 serviceframework::service::protocol::deduce( std::move( parameter)),
                                 []( auto& state){ return casual::domain::manager::configuration::get( state);},
                                 state);
                           };
                        }

                        auto put( manager::State& state)
                        {
                           return [&state]( common::service::invoke::Parameter&& parameter)
                           {
                              auto protocol = serviceframework::service::protocol::deduce( std::move( parameter));

                              casual::configuration::domain::Manager domain;
                              protocol >> CASUAL_NAMED_VALUE( domain);

                              return serviceframework::service::user( 
                                 std::move( protocol),
                                 [&](){ return casual::domain::manager::configuration::put( state, std::move( domain));});
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
                        common::service::category::admin()
                     },
                     { service::name::scale::instances,
                           local::service::scale( state),
                           common::service::transaction::Type::none,
                           common::service::category::admin()
                     },
                     { service::name::restart::instances,
                           local::service::restart( state),
                           common::service::transaction::Type::none,
                           common::service::category::admin()
                     },
                     { service::name::shutdown,
                           local::service::shutdown( state),
                           common::service::transaction::Type::none,
                           common::service::category::admin()
                     },
                     { service::name::configuration::persist,
                           local::service::configuration::persist( state),
                           common::service::transaction::Type::none,
                           common::service::category::admin()
                     },
                     { service::name::configuration::get,
                           local::service::configuration::get( state),
                           common::service::transaction::Type::none,
                           common::service::category::admin()
                     },
                     { service::name::configuration::put,
                           local::service::configuration::put( state),
                           common::service::transaction::Type::none,
                           common::service::category::admin()
                     },
                     { service::name::environment::set,
                           local::service::environment::set( state),
                           common::service::transaction::Type::none,
                           common::service::category::admin()
                     }
               }};
            }
         } // admin
      } // manager
   } // gateway
} // casual
