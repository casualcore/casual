//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/manager/admin/server.h"
#include "domain/manager/admin/vo.h"
#include "domain/manager/handle.h"
#include "domain/manager/persistent.h"
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
                     std::vector< vo::scale::Instances> instances( manager::State& state, std::vector< vo::scale::Instances> instances)
                     {
                        std::vector< vo::scale::Instances> result;

                        auto scale_entities = [&]( auto& instance, auto& entities){

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

                  namespace set
                  {
                     auto environment( manager::State& state, const vo::set::Environment& environment)
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
                     common::service::invoke::Result state( common::service::invoke::Parameter&& parameter, manager::State& state)
                     {
                        return serviceframework::service::user( 
                           serviceframework::service::protocol::deduce( std::move( parameter)), 
                           []( auto& state){ return casual::domain::transform::state( state);}, 
                           state);
                     }


                     common::service::invoke::Result scale( common::service::invoke::Parameter&& parameter, manager::State& state)
                     {
                        auto protocol = serviceframework::service::protocol::deduce( std::move( parameter));

                        std::vector< vo::scale::Instances> instances;
                        protocol >> CASUAL_MAKE_NVP( instances);

                        return serviceframework::service::user( std::move( protocol), &scale::instances, state, instances);
                     }


                     common::service::invoke::Result shutdown( common::service::invoke::Parameter&& parameter, manager::State& state)
                     {
                        return serviceframework::service::user( 
                           serviceframework::service::protocol::deduce( std::move( parameter)), 
                           &handle::shutdown, state);
                     }

                     common::service::invoke::Result persist( common::service::invoke::Parameter&& parameter, manager::State& state)
                     {
                        return serviceframework::service::user( 
                           serviceframework::service::protocol::deduce( std::move( parameter)),
                           []( auto& state){ return persistent::state::save( state);},
                           state);
                     }

                     namespace set
                     {
                        common::service::invoke::Result environment( common::service::invoke::Parameter&& parameter, manager::State& state)
                        {
                           auto protocol = serviceframework::service::protocol::deduce( std::move( parameter));

                           vo::set::Environment environment;
                           protocol >> CASUAL_MAKE_NVP( environment);

                           return serviceframework::service::user(
                              std::move( protocol),
                              &local::set::environment,
                              state, 
                              environment);
                        }
                     } // set


                  } // service
               } // <unnamed>
            } // local

            common::server::Arguments services( manager::State& state)
            {
               return { {
                     { service::name::state,
                        std::bind( &local::service::state, std::placeholders::_1, std::ref( state)),
                        common::service::transaction::Type::none,
                        common::service::category::admin()
                     },
                     { service::name::scale::instances,
                           std::bind( &local::service::scale, std::placeholders::_1, std::ref( state)),
                           common::service::transaction::Type::none,
                           common::service::category::admin()
                     },
                     { service::name::shutdown,
                           std::bind( &local::service::shutdown, std::placeholders::_1, std::ref( state)),
                           common::service::transaction::Type::none,
                           common::service::category::admin()
                     },
                     { service::name::configuration::persist,
                           std::bind( &local::service::persist, std::placeholders::_1, std::ref( state)),
                           common::service::transaction::Type::none,
                           common::service::category::admin()
                     },
                     { service::name::set::environment,
                           std::bind( &local::service::set::environment, std::placeholders::_1, std::ref( state)),
                           common::service::transaction::Type::none,
                           common::service::category::admin()
                     }
               }};
            }
         } // admin
      } // manager
   } // gateway
} // casual
