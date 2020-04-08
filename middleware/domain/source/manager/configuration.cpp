//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "domain/manager/configuration.h"
#include "domain/manager/state/create.h"
#include "domain/transform.h"
#include "domain/manager/manager.h"
#include "domain/manager/task/create.h"

#include "configuration/domain.h"

namespace casual
{
   using namespace common;
   namespace domain
   {
      namespace manager
      {
         namespace configuration
         {
   
            State state( const Settings& settings)
            {
               auto state = casual::domain::transform::state( casual::configuration::domain::get( settings.configurationfiles));
               state.bare = settings.bare;

               if( settings.event.ipc)
               {
                  common::message::event::subscription::Begin request;
                  request.process.ipc = settings.event.ipc;
                  state.event.subscription( request);
               }

               return state;
            }


            namespace local
            {
               namespace
               {
                  namespace transform
                  {
                     auto membership( const State& state)
                     {
                        return [&state]( auto id)
                        {
                           return state.group( id).name;
                        };
                     }

                     auto group( const State& state)
                     {
                        return [&state]( const state::Group& value)
                        {
                           casual::configuration::Group result;

                           result.name = value.name;
                           result.note = value.note;
                           result.dependencies = algorithm::transform( value.dependencies, transform::membership( state));
                           result.resources = value.resources;
                           
                           return result;
                        };
                     }

                     template< typename R, typename P> 
                     R process( const State& state, const P& value)
                     {
                        R result;

                        result.alias = value.alias;
                        result.path = value.path;
                        result.note = value.note;
                        result.restart = value.restart;
                        result.arguments = value.arguments;
                        result.instances = value.instances.size();
                        result.memberships = algorithm::transform( value.memberships, transform::membership( state));

                        if( ! value.environment.variables.empty())
                        {
                           casual::configuration::Environment environment;
                           environment.variables = casual::configuration::environment::transform( value.environment.variables);
                           result.environment = std::move( environment);
                        }

                        return result;
                     }

                     auto server( const State& state)
                     {
                        return [&state]( const state::Server& value)
                        {
                           auto result = transform::process< casual::configuration::Server>( state, value);
                           

                           return result;
                        };
                     }

                     auto executable( const State& state)
                     {
                        return [&state]( const state::Executable& value)
                        {
                           auto result = transform::process< casual::configuration::Executable>( state, value);

                           return result;
                        };
                     }
                  } // transform

                  namespace extract
                  {
                     //! @return a tuple with intersected and complement of the configuration (compared to state)
                     auto interesection( const State& state, casual::configuration::domain::Manager configuration)
                     {
                        casual::configuration::domain::Manager intersection;
                        casual::configuration::domain::Manager complement;

                        auto extract = []( auto& source, auto& lookup, auto predicate, auto& interesected, auto& complemented)
                        {
                           auto split = algorithm::intersection( source, lookup, predicate);
                           algorithm::move( std::get< 0>( split), interesected);
                           algorithm::move( std::get< 1>( split), complemented);
                        };

                        auto alias_equal = []( auto& lhs, auto& rhs){ return lhs.alias == rhs.alias;};

                        // take care of servers and executables
                        extract( configuration.servers, state.servers, alias_equal, intersection.servers, complement.servers);
                        extract( configuration.executables, state.executables, alias_equal, intersection.executables, complement.executables);

                        auto name_equal = []( auto& lhs, auto& rhs){ return lhs.name == rhs.name;};

                        extract( configuration.groups, state.groups, name_equal, intersection.groups, complement.groups);

                        return std::make_tuple( std::move( intersection), std::move( complement));
                     }
                  } // extract

                  namespace task
                  {
                     auto complement( State& state, const casual::configuration::domain::Manager& configuration)
                     {
                        auto servers = casual::domain::transform::executables( configuration.servers, state.groups);
                        auto executables = casual::domain::transform::executables( configuration.executables, state.groups);

                        algorithm::append( servers, state.servers);
                        algorithm::append( executables, state.executables);

                        auto task = manager::task::create::scale::aliases( "configuration put", state::create::boot::order( state, servers, executables));

                        // add, and possible start, the tasks
                        return state.tasks.add( std::move( task));
                     }
                  } // task

               } // <unnamed>
            } // local


            casual::configuration::domain::Manager get( const State& state)
            {
               Trace trace{ "domain::manager::configuration::get"};

               casual::configuration::domain::Manager result;

               result.groups = algorithm::transform( state.groups, local::transform::group( state));
               result.servers = algorithm::transform( state.servers, local::transform::server( state));
               result.executables = algorithm::transform( state.executables, local::transform::executable( state));

               return result;
            }


            std::vector< common::Uuid> put( State& state, casual::configuration::domain::Manager configuration)
            {
               Trace trace{ "domain::manager::configuration::put"};
               log::line( verbose::log, "configuration: ", configuration);

               auto interesection = local::extract::interesection( state, std::move( configuration));
               log::line( verbose::log, "interesection: ", std::get< 0>( interesection));
               log::line( verbose::log, "complement: ", std::get< 1>( interesection));

               return { local::task::complement( state, std::get< 1>( interesection))};
            }

         } // configuration
      } // manager
   } // domain



} // casual
