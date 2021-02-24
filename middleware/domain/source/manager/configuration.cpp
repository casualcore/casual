//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "domain/manager/configuration.h"
#include "domain/manager/state.h"
#include "domain/manager/state/create.h"
#include "domain/transform.h"
#include "domain/manager/task/create.h"

#include "configuration/model.h"
#include "configuration/model/load.h"
#include "configuration/model/transform.h"

namespace casual
{
   using namespace common;
   namespace domain
   {
      namespace manager
      {
         namespace configuration
         {
   
            namespace local
            {
               namespace
               {

                  //! @return a tuple with intersected and complement of the configuration (compared to state)
                  auto interesection( casual::configuration::Model current, casual::configuration::Model wanted)
                  {
                     casual::configuration::Model intersection;
                     casual::configuration::Model complement;

                     auto extract = []( auto& source, auto& lookup, auto predicate, auto& interesected, auto& complemented)
                     {
                        auto split = algorithm::intersection( source, lookup, predicate);
                        algorithm::move( std::get< 0>( split), interesected);
                        algorithm::move( std::get< 1>( split), complemented);
                     };

                     auto alias_equal = []( auto& lhs, auto& rhs){ return lhs.alias == rhs.alias;};

                     // take care of servers and executables
                     extract( wanted.domain.servers, current.domain.servers, alias_equal, intersection.domain.servers, complement.domain.servers);
                     extract( wanted.domain.executables, current.domain.executables, alias_equal, intersection.domain.executables, complement.domain.executables);

                     auto name_equal = []( auto& lhs, auto& rhs){ return lhs.name == rhs.name;};

                     extract( wanted.domain.groups, current.domain.groups, name_equal, intersection.domain.groups, complement.domain.groups);

                     return std::make_tuple( std::move( intersection), std::move( complement));
                  }

                  namespace task
                  {
                     auto complement( State& state, const casual::configuration::Model& model)
                     {
                        auto servers = casual::domain::transform::alias( model.domain.servers, state.groups);
                        auto executables = casual::domain::transform::alias( model.domain.executables, state.groups);

                        algorithm::append( servers, state.servers);
                        algorithm::append( executables, state.executables);

                        auto task = manager::task::create::scale::aliases( "model put", state::create::boot::order( state, servers, executables));

                        // add, and possible start, the tasks
                        return state.tasks.add( std::move( task));
                     }
                  } // task

               } // <unnamed>
            } // local


            casual::configuration::Model get( const State& state)
            {
               Trace trace{ "domain::manager::configuration::get"};
               return transform::model( state);
            }


            std::vector< common::Uuid> put( State& state, casual::configuration::Model wanted)
            {
               Trace trace{ "domain::manager::configuration::put"};
               log::line( verbose::log, "model: ", wanted);

               auto interesection = local::interesection( configuration::get( state), std::move( wanted));

               log::line( verbose::log, "interesection: ", std::get< 0>( interesection));
               log::line( verbose::log, "complement: ", std::get< 1>( interesection));

               return { local::task::complement( state, std::get< 1>( interesection))};
            }

         } // configuration
      } // manager
   } // domain



} // casual
